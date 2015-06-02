#!/usr/bin/env perl

use strict;
use warnings;
#1C7E:00000607  push si                                                EAX:00000003 EBX:00000710 ECX:0000021E EDX:00000000 ESI:00000001 EDI:00000003 EBP:0000947C ESP:0000947C DS:5C4E ES:20DC FS:0000 GS:0000 SS:5C4E CF:1 ZF:0 SF:1 OF:0 AF:1 PF:1 IF:1
#1C7E:00000608  mov  byte [0A8F],01             ds:[0A8F]=0001         EAX:00000003 EBX:00000710 ECX:0000021E EDX:00000000 ESI:00000001 EDI:00000003 EBP:0000947C ESP:0000947A DS:5C4E ES:20DC FS:0000 GS:0000 SS:5C4E CF:1 ZF:0 SF:1 OF:0 AF:1 PF:1 IF:1

if ($#ARGV != 0) {
    print "Provide a single filename as an arguement.\n";
    exit(1);
}

open(my $infile, "<", $ARGV[0]) or die "Cannot open ".$ARGV[0].": $!\n";

my %instructions;
my %counts;

sub print_insts($) {
    my ($depth) = @_;
    my($lastseg, $func_seg, $func_off) = ('0000', '0000', '0000');
    #print "$depth \n";
    while(my $line = <$infile>) {

        #Read line, parse relevant address data
        $line =~ s/Mouse/Int 33 Mouse/;
        $line =~ /([0-9A-F]{4}):([0-9A-F]{8})  ((.*)[A-Za-z0-9\)\]])\s+(E)*AX/;
        my ($seg, $addr, $paddr, $data) = ($1, $2, hex($1) * 16 + hex($2), $3);

        #'0000' is a magic segment number for an initial call, when I don't know what the seg will be
        if($lastseg  eq '0000') {
            $lastseg = $seg;
            $func_off = $addr;
        }

        #evidence of unexpected code jump
        if($lastseg ne $seg) { #time-based interrupt called?
            #print "Suspected interrupt. Increasing depth.\n";
            seek($infile, -length($line), 1); #Put the line back
            print_insts($depth + 1);
            next;
        }

        #store the instruction in the hash of the function it belongs to
        $instructions{"$seg:$func_off"}{"$seg:$addr"} = $data;

        #construct the output string
        my $outstr = "$seg : $addr  $data";
        foreach my $x (0..$depth) {
            $outstr = '    ' . $outstr;
        }
        foreach my $x (0..(120 - length $outstr)) {
            $outstr = $outstr . ' ';
        }
        $outstr.="($paddr)\n";
        if($depth > 90) {
            print STDERR $outstr;
        }

        #Update count of how many times the instruction has been seen
        if(defined($counts{$paddr})) {
            $counts{$paddr} = $counts{$paddr} + 1;
        }
        else {
            $counts{$paddr} = 1;
        }

        #If the instruction would change the stack frame, mirror that by recursing deeper
        if($line =~ 'call ' || $line =~ 'int( ){1,2}[0-9A-F]{2}') {
            #print "First inst of call/int should be next\n";
            print_insts($depth + 1);
        }
        elsif($data =~ 'ret') {
            return;
        }
        #Expected change of segment. Don't want to trigger a false deepening of the stack =)
        elsif($data =~ 'jmp  ([0-9A-F]{4}):[0-9A-F]{4}') {
            $lastseg = $1;
        }
    }
}

while(! eof($infile)) {
    print_insts(0);
    if(! eof($infile)) {
        print "Oops, start of log wasn't at top of stack (Returned from recursion level that we thought was the top)\n";
        %instructions=();
        %counts=();
    }
}

my @count_list;
for my $k (keys %counts) {
    push @count_list, sprintf("%05X: %d", $k, $counts{$k});
#    printf("%10X: %d\n",$k,$counts{$k});
}

@count_list = sort(@count_list);
for my $addrs (@count_list) {
    print "$addrs\n";
}

for my $functs (sort(keys %instructions)) {
    my $f_ref = $instructions{$functs};
    for my $insts (sort(keys %$f_ref)) {
        print "$insts  \"".$instructions{$functs}{$insts}."\"\n";
    }
    print "\n";
}
