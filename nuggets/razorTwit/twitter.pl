#!/usr/bin/perl
use strict;
use warnings;
use diagnostics;
use English;

use Net::Twitter;
use File::Spec;
use Storable;
use Net::Stomp;
use Config::General;
use JSON;
use Data::Dumper;
use Proc::Daemon;

my $conf =  new Config::General("twitter.conf");
my %config = $conf->getall;
my %consumer_tokens = (
    consumer_key    => "0x6wA1zoC9N6opjpPajYw",
    consumer_secret => "mhO8AfRrmUvkClTYgK2qqMhojvf2kwLCymrppnLfz0",
    );

# $datafile = oauth_desktop.dat
my (undef, undef, $datafile) = File::Spec->splitpath($0);
$datafile =~ s/\..*/.dat/;

my $nt = Net::Twitter->new(traits => [qw/API::REST OAuth/], %consumer_tokens);
my $access_tokens = eval { retrieve($datafile) } || [];

if ( @$access_tokens ) {
    $nt->access_token($access_tokens->[0]);
    $nt->access_token_secret($access_tokens->[1]);
}
else {
    my $auth_url = $nt->get_authorization_url;
    print " Authorize this application at: $auth_url\nThen, enter the PIN# provided to contunie: ";

    my $pin = <STDIN>; # wait for input
    chomp $pin;

    # request_access_token stores the tokens in $nt AND returns them
    my @access_tokens = $nt->request_access_token(verifier => $pin);

    # save the access tokens
    store \@access_tokens, $datafile;
}
eval { Proc::Daemon::Init; };
if ($@) {
       die("Unable to start daemon:  $@");
}
my $continue = 1;

my $stomp = Net::Stomp->new( { hostname => $config{"mq"}->{"host"}, port => $config{"mq"}->{"port"} } );
$stomp->connect( { login => $config{"mq"}->{"user"}, passcode => $config{"mq"}->{"pass"} } );
$stomp->subscribe(
  {   destination             => '/topic/Alert.'.$config{"message"}->{"filter"},
      'ack'                   => 'client',
      'activemq.prefetchSize' => 1
  }
);



$SIG{TERM} = sub { $continue = 0 };
while ($continue) {
    my $frame = $stomp->receive_frame();
    $stomp->ack( { frame => $frame } );
    my $msg = decode_json $frame->body;
    my $out = "Bad ".$msg->{"Message"}." ".$msg->{"Block"}->{"Id"}->{"Size"}." ".$msg->{"Block"}->{"Id"}->{"Hash"}->{"Value"};
    if (length($out) >= 140)
    {
        $out = substr($out,0,135)."....";
    }
    $nt->update($out);
}

$stomp->disconnect;
