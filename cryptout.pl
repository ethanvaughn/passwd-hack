#!/usr/bin/perl
# returns an encrypted version of the string passed on the command-line suitable
# for use in /etc/passwd and /etc/smbpasswd
srand (time());
my $randletter = "(int (rand (26)) + (int (rand (1) + .5) % 2 ? 65 : 97))";
my $salt = sprintf("%c%c", eval $randletter, eval $randletter);
my $plaintext = shift;
my $crypttext = crypt ($plaintext, $salt);

print "${crypttext}\n";
