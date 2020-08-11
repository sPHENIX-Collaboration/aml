#! /usr/bin/perl

#$ENV{"DISPLAY"} = "va053.phenix.bnl.gov:0.0";


use Tk;
use Getopt::Long;
GetOptions('help','display:s','large','token:s');

if ($opt_help)
{
    print" 
    loggerdisplay.pl [ options ... ]
    --help                      this text
    --display=<remote_display>  display on the remote display, e.g va053:0.
    --large                     size suitable for showing on an overhead display

\n";
    exit(0);
}

if ($opt_display)
{
    $ENV{"DISPLAY"} = $opt_display;

}


if ($opt_large)
{
    $titlefontsize=20;
    $fontsize=13;

#    Four box setup
#    $position{"phnxbox0"} = "+5+5";
#    $position{"phnxbox1"} = "+5+220";
#    $position{"phnxbox2"} = "+5+440";
#    $position{"phnxbox3"} = "+5+660";

#   Six box setup
#    $position{"phnxbox0"} = "+5+5";
#    $position{"phnxbox1"} = "+5+160";
#    $position{"phnxbox2"} = "+5+315";
#    $position{"phnxbox3"} = "+5+470";
#    $position{"phnxbox4"} = "+5+625";
#    $position{"phnxbox5"} = "+5+780";

#   Six box setup
    
#     $position{"phnxbox0"} = "+5+0";
#     $position{"phnxbox1"} = "+5+128";
#     $position{"phnxbox2"} = "+5+256";
#     $position{"phnxbox3"} = "+5+384";
#     $position{"phnxbox4"} = "+5+512";
#     $position{"phnxbox5"} = "+5+640";
#     $position{"phnxbox6"} = "+5+768";
#     $position{"phnxbox7"} = "+5+896";
    $position{"phnxbox0"} = "1270x128+5+0";
    $position{"phnxbox1"} = "1270x128+5+128";
    $position{"phnxbox2"} = "1270x128+5+256";
    $position{"phnxbox3"} = "1270x128+5+384";
    $position{"phnxbox4"} = "1270x128+5+512";
    $position{"phnxbox5"} = "1270x128+5+640";
    $position{"phnxbox6"} = "1270x128+5+768";
    $position{"phnxbox7"} = "1270x128+5+896";


    $xpadding =250;
    $ypadding =2;
    $xipadding = 40;


}
else
{
    $titlefontsize=12;
    $fontsize=10;
    $position{"phnxbox0"} = "+20+20";
    $position{"phnxbox2"} = "+630+20";
    $xpadding =60;
    $ypadding =5;
    $xipadding = 10;
}

$amltoken{"phnxbox0"} = "AML";
$amltoken{"phnxbox1"} = "AML";
$amltoken{"phnxbox2"} = "AML";
$amltoken{"phnxbox3"} = "AML";
$amltoken{"phnxbox4"} = "AML";
$amltoken{"phnxbox5"} = "AML";
$amltoken{"phnxbox6"} = "AML3";
$amltoken{"phnxbox7"} = "AML3";

# get the full hostname, and just the name portion
$fullhostname = `hostname`;
chop $fullhostname;
@x = split(/\./, $fullhostname);
$hostname = $x[0];

$mb_so_far = -1;


#if ( ! ( $hostname =~ /phnxbox4/ || $hostname =~ /phnxbox5/ ) )
#{
#    die "You can run this on phnxbox4 or 5 only\n";
#}

$alive = 0;
$mb = 0;
$currentfile="";
$currentsegment=-1;
$nevents = 0;



$mw = MainWindow->new();
$mw->geometry($position{$hostname});

$mw->title("AML display for $hostname");

$frame1 = $mw->Frame()->pack(-side=>"top", -fill=>"x");


#$frame2 = $mw->Frame()->pack(-side=>"top", -fill=>'x');

$frame3 = $mw->Frame()->pack(-side=>bottom, -expand=>1, -fill=>'both');





$label{"TOP"} = $frame1->Label(-text => $hostname, 
			      -font => ['arial', $titlefontsize, 'bold'],
			      -relief => 'groove', -bg => 'lightblue', -fg=>'blue')->pack(-side=>top, 
											  -fill => 'both', 
											  -ipady => $ypadding, 
											  -ipadx => $xpadding);




$framea = $frame3->Frame()->pack(-side=>"left", -fill=>'both');
$frameb = $frame3->Frame()->pack(-side=>"left", -fill=>'both');
$framec = $frame3->Frame()->pack(-side=>"left", -fill=>'both', -expand=>1 );


$label{"Filed"} = $framea->Label(-text => "Current Filename:", 
			       -font => ['arial', $fontsize, 'bold'],
			       -relief => 'groove')->pack(-fill=>'both', -expand=>1, -ipadx => 10, -ipady => 5);


$label{"Alive"} = $framec->Label(-text => "", 
			       -font => ['arial', $titlefontsize, 'bold'],
			       -relief => 'groove')->pack(-fill=>'both', -expand=>1, -ipadx => 10, -ipady => 5);



$label{"MBd"} = $framea->Label(-text => "Events & MB written:", 
			       -font => ['arial', $fontsize, 'bold'],
			       -relief => 'groove')->pack(-fill=>'both', -expand=>1, -ipadx => 10, -ipady => 5);




$label{"File"} = $frameb->Label(-text => "", 
			       -font => ['arial', $fontsize, 'bold'],
			       -relief => 'groove')->pack(-fill=>'both', -expand=>1, -ipadx => $xipadding, -ipady => 5);



$label{"MB"} = $frameb->Label(-text => "", 
			       -font => ['arial', $fontsize, 'bold'],
			       -relief => 'groove')->pack(-fill=>'both', -expand=>1, -ipadx => $xipadding, -ipady => 5);


# update once to fill all the values in
update();

#and schedule an update every 5 seconds

$mw->repeat(10000,\&update);

MainLoop();




sub update()
{
    $alive = 1;
    $error = 0;
    $mb = 0;
    $currentfile="";
    $currentsegment=-1;
    
    if ($opt_token)
    {
      $amlname= $opt_token; 
    }
    else
    {
      $amlname= $amltoken{$hostname};
    }

    open (R, "aml_display -t $amlname |");

    while ( <R> ) 
    {
	chop;
	
	if ( $_ =~ /not running/ ) { $alive = 0; }
	
	if ( $_ =~ /Mbytes/ ) 
	{ 
	    @v = split (/:/,$_);
	    $mb = $v[1];
	    if ( $mb == $mb_so_far) { $alive = 0; }
	    $mb_so_far = $mb;

	}
	
	if ( $_ =~ /current filename/ )
	{
	    @v = split (/:/,$_);
	    $currentfile = $v[1];
	}
	
	if ( $_ =~ /sequence/ )
	{
	    @v = split (/\W/,$_);
	    $currentsegment = $v[5];
	}

	if ( $_ =~ /Events in run/ )
	{
	    @v = split (/\W+/,$_);
	    $nevents = $v[3];
	}

	if ( $_ =~ /Error:/ )
	{
	  $errmsg=$_;
	  $error = 1;

	}

    }

    close (R);

    
#    print " $alive $mb $currentfile  $currentsegment \n";

    $df_command = "df -lP | sort -n";
    $df_summary = $hostname."   ";

    open( DF,"$df_command|" ) or die "Failure in df command";
    while ( <DF> ) 
    {
	($device, $blocks, $used, $available, $percent, $mountpoint) = split;
	if ( $mountpoint =~ m/\/a$|\/b$|\/c$|\/d$/ ) 
	{
	    $mp{$mountpoint} = $percent;
	}


    }
    close( DF );
    for $m ( sort keys %mp )
    {
	
	$df_summary = $df_summary.$m." ".$mp{$m}."   ";
    }

    $label{"TOP"}->configure(-text => $df_summary);
    $label{"File"}->configure(-text => $currentfile);
    $label{"MB"}->configure(-text => "$nevents  $mb");



    if ( $error)
    {
      $label{"Alive"}->configure(-bg=>"red", -fg=>'black', -text=>$errmsg);
    }
    else
    {
      
      if ($alive)
      {
	$label{"Alive"}->configure(-bg=>"lightgreen", -fg=>'black', -text=>"AML is alive");
      }

      else
      {
	$label{"Alive"}->configure(-bg=>"lightgrey", -fg=>'black', -text=>"No data written by AML");
      }	
      
    }
}
