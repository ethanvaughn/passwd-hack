#!/usr/bin/perl

sub getpass {
    my $prompt = shift || "Enter password:";

    open STDIN, "/dev/tty" or warn "couldn't open /dev/tty $!\n";
    system "stty raw; stty -echo;";
 
    print STDERR $prompt;
    while ($c = getc(STDIN) and $c ne "\n" and $c ne "\r") {
        $password .= $c;
    }

   system("stty sane");
   sleep(1);
 
    return $password;
}
my $p = getpass;
print "\nYou entered: `$p'\n"; 
