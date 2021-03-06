#!/usr/bin/perl

# parameter: 1. Number of events
#            2. Number of first event 
#
# for template configuration files, assume template_cfg.py

#chomp $ARGV[0];
chomp $ARGV[2];

# @filelist = `ls $ARGV[0]`;

$number_of_events = $ARGV[0];
$first_event = $ARGV[1];

@joblist = ();

###########################################
# PARAMETERS : Edit for desired values

$run = 13;
$event_per_run = 100;
$run_per_job = 1;

$tag="pythia_z2muons_d200800919";

$rerun=0;
###########################################


`./makeProdDir ${tag}`;

$last_event = $first_event + $number_of_events - 1;

@events = `seq $first_event $last_event`;

$work_dest = "DataFrom" . $first_event . "To" . $last_event;

`mkdir $work_dest`;

$dummy = 0;

$event = $first_event;
$folder = (($first_event - 1) / $event_per_run) + 1;

while($event <= $last_event){

    $grandrunnum = `printf "%06d" $folder`;

    if($rerun==0){
	$random = int(rand(999999));
	
	`cat cfg1.py |sed "s/__MAXEVENTS__/$event_per_run/g" | sed "s/__SKIP__/$skip/g" | sed "s/__OUTPUT__/${grandrunnum}_1.root/g" | sed "s/__RANDOM__/$random/g" | sed "s/__MIX__/$backgroundlist[$background]/g" | sed "s/__INPUT__/$signalfile/g" | sed "s/__LIST__/$grandrunnum/g" | sed "s/__FIRSTEVENT__/$event/g" | sed "s/__RUN__/$run/g" >> ${grandrunnum}_cfg1.py`;
	`cat cfg2.py | sed "s/__OUTPUT__/${grandrunnum}_2.root/g" | sed "s/__RANDOM__/$random/g" | sed "s/__MIX__/$backgroundlist[$background]/g" | sed "s/__INPUT__/${grandrunnum}_1.root/g" >> ${grandrunnum}_cfg2.py`;
	`cat cfg3.py | sed "s/__OUTPUT__/${grandrunnum}_3.root/g" | sed "s/__RANDOM__/$random/g" | sed "s/__MIX__/$backgroundlist[$background]/g" | sed "s/__INPUT__/${grandrunnum}_2.root/g" >> ${grandrunnum}_cfg3.py`;
	`cat cfg4.py | sed "s/__OUTPUT__/${grandrunnum}_4.root/g" | sed "s/__RANDOM__/$random/g" | sed "s/__MIX__/$backgroundlist[$background]/g" | sed "s/__INPUT__/${grandrunnum}_3.root/g" >> ${grandrunnum}_cfg4.py`;
	`cat cfg5.py | sed "s/__OUTPUT__/${grandrunnum}_5.root/g" | sed "s/__RANDOM__/$random/g" | sed "s/__MIX__/$backgroundlist[$background]/g" | sed "s/__INPUT__/${grandrunnum}_4.root/g" >> ${grandrunnum}_cfg5.py`;
    }
    $event = $event + $event_per_run;
    push @joblist, $grandrunnum;
    $folder++;
}

`cp condor_backup condor`;
$size = scalar @joblist;

$index = 0;
for($index = 0; $index < scalar @joblist; $index = $index + $run_per_job)
{
    
    if($rerun==0){
	`mkdir $work_dest/$joblist[$index]`;
	`cat run_template.pl | sed "s/__TAG__/${tag}/g" > $work_dest/$joblist[$index]/run_$joblist[$index].pl`;
	`cp check.pl $work_dest/$joblist[$index]/`;
	`cp finalize.sh $work_dest/$joblist[$index]/`;
    }else{
	`rm $work_dest/$joblist[$index]/*.*.*`;
        `rm $work_dest/$joblist[$index]/*.hist`;
        `rm $work_dest/$joblist[$index]/*.err`;
        `rm $work_dest/$joblist[$index]/*.out`;
        `rm $work_dest/$joblist[$index]/*.root`;
    }
	
    @inputfilelist = ();
    
    $filelist = "";
    
    for($index2 = 0; $index2 < $run_per_job; $index2++)
    {
	if($index2 + $index < scalar @joblist)
	{
	    if($rerun==0){
		`mv $joblist[$index2+$index]_cfg1.py $work_dest/$joblist[$index]`;
		`mv $joblist[$index2+$index]_cfg2.py $work_dest/$joblist[$index]`;
		`mv $joblist[$index2+$index]_cfg3.py $work_dest/$joblist[$index]`;
		`mv $joblist[$index2+$index]_cfg4.py $work_dest/$joblist[$index]`;
		`mv $joblist[$index2+$index]_cfg5.py $work_dest/$joblist[$index]`;
	    }	    
	    push @inputfilelist, $joblist[$index+$index2];
	    
	    if($index2 > 0)
	    {
		$filelist = $filelist . ", ";
	    }
	    
	    $filelist = $filelist . "\\\"file:" . $joblist[$index+$index2] . "_4.root\\\"";
	}
    }
        
    $merged_output="merged.root";
    `cat merge_cfg.py | sed "s/__OUTPUT__/$merged_output/g" | sed "s/__INPUT__/$filelist/g" > $work_dest/$joblist[$index]/merge_cfg.py`;
    
    $filestring = "";
    $commafilestring = "";
    foreach $argument (@inputfilelist)
    {
	chomp $argument;
	if(length $commafilestring != 0)
	{
	    $filestring = $filestring . " ";
	    $commafilestring = $commafilestring . ", ";
	}

	$filestring = $filestring . $argument;
        $commafilestring = $commafilestring . $argument . "_cfg1.py, " . $argument . "_cfg2.py, " . $argument . "_cfg3.py," . $argument . "_cfg4.py," . $argument . "_cfg5.py,";
    }

    $commafilestring = $commafilestring . ", merge_cfg.py, run_$joblist[$index].pl, run_$joblist[$index].sh, finalize.sh";

    `cat run_template.sh | sed "s/__INTAG__/$signal/g" >> $work_dest/$joblist[$index]/run_$joblist[$index].sh`;
    `echo "perl check.pl $filestring 1> check.out 2> check.err &" >> $work_dest/$joblist[$index]/run_$joblist[$index].sh`;
    `echo "perl run_$joblist[$index].pl $filestring" >> $work_dest/$joblist[$index]/run_$joblist[$index].sh`;

    `echo "./finalize.sh $tag $joblist[$index]" >> $work_dest/$joblist[$index]/run_$joblist[$index].sh`;

    `echo "edm=$do_edm" >>$work_dest/$joblist[$index]/run_$joblist[$index].sh`;
    `echo "hidout=hid_$joblist[$index].root" >> $work_dest/$joblist[$index]/run_$joblist[$index].sh`;
    `echo "edmout=edm_$joblist[$index].root" >> $work_dest/$joblist[$index]/run_$joblist[$index].sh`;
    `echo "Initialdir           = $work_dest/$joblist[$index]" >> condor`;
    `echo "Executable           = $work_dest/$joblist[$index]/run_$joblist[$index].sh" >> condor`;
    `echo "Arguments            = " >> condor`;
    `echo "transfer_input_files = $commafilestring, check.pl" >> condor`;
    `echo "Queue" >> condor`;
    `echo >> condor`;

}

`cp condor ${work_dest}`;






