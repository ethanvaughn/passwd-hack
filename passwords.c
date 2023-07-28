/*
 * cpasswords.c
 * Copyright N. Dean Pentcheff  1998
 * University of South Carolina
 * [PRIVACY PROTECTION]
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the same terms as Perl itself (see http://www.perl.com).
 *
 * Change both the Unix and SMB passwords for a user.
 * To work, must be installed SUID-root.
 * If called interactively (from a tty), is slightly verbose, and uses
 *   the standard getpass() routine to query for and confirm a password.
 * If called noninteractively, expects the password (once) on stdin.
 *
 * Customize the locations of the standard Unix and SMB password programs
 * in the "#defines" near the top (do NOT be tempted to add code to make
 *   these changeable from command-line arguments: these programs will
 *   be run as root!).  If your paths are obscenely long, examine the
 *   size of STRLEN to make sure it will accomodate them.
 * The sleep()s in the actual pwd-changing routines appeared to be necessary
 *   in some early tests I did with the PAM-passwd program on Linux.  I'm
 *   not convinced they're always necessary.  Delays in a pwd-changing
 *   program aren't a bad idea anyway, so I've left them in.
 */
#include <fcntl.h> #include <sys/ioctl.h> #include <pwd.h> #include <errno.h> #include <stdio.h> #include <unistd.h> #include <string.h>  #define  PASSWD    "/usr/bin/passwd"
#define  SMBPASSWD "/usr/local/bin/smbpasswd"
#define  PROMPT1   "Type a new password: "
#define  PROMPT2   "Type the same password again: "
#define  STRLEN    1024

int main (void)
{
  int    fd;
  int    status;
  struct passwd *pwentry;
  char   new[STRLEN];
  char   cmd[STRLEN];
  char   *cp;
  FILE   *cmdpipe;
  FILE   *mystderr;

  /* what's my username? */
  if ((pwentry = getpwuid(getuid())) == NULL) {
    fprintf(stderr, "Failed getting name entry for UID=%d, exiting...\n",
	    getuid());
    exit(1);
  }

  /* do we have the appropriate permissions? */
  if (geteuid() != 0) {
    fprintf(stderr, "This program cannot run unless it is SUID-root, "
	    "exiting...\n");
    exit(1);
  }

  /* get a password and strip any trailing cr/lf stuff */
  if (isatty(0)) { /* interactive, so use a no-echo prompt twice */
    cp = getpass(PROMPT1);
    strncpy(new, cp, STRLEN);
    cp = getpass(PROMPT2);
    if (strcmp(new, cp)) {
      fprintf(stderr, "The two versions don't match, exiting...\n");
      exit(1);
    }
  } else { /* noninteractive, so just get it from stdin */
    if (read(0, new, STRLEN) <= 0) {
      fprintf(stderr, "Failed to read a new password, exiting...\n");
      exit(1);
    }
  }
  for (cp=new;  *cp!='\n' && *cp!='\r' && cp-new<STRLEN-1;  ++cp)
    ;
  *cp = '\0';
  if (strlen(new) <= 0) {
    fprintf(stderr, "No password entered, exiting...\n");
    exit(1);
  }

  /* get a private stderr, then close stderr/stdout to silence pwd programs */
  if ((fd = dup(2)) < 0) {
    fprintf(stderr, "Strange!  Couldn't dup error-output fd, exiting...\n");
    exit(1);
  }
  if ((mystderr = fdopen(fd, "w")) == NULL) {
    fprintf(stderr, "Strange!  Couldn't fdopen new stderr fd, exiting...\n");
    exit(1);
  }
  close(1);
  close(2);

  /* detach from controlling tty to convince smbpasswd to read stdin for pw */
  if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
    if (ioctl(fd, TIOCNOTTY) < 0) {
      fprintf(mystderr, "Failed to detach from /dev/tty: %s, exiting...\n",
	      strerror(errno));
      exit(1);
    }
    close(fd);
  }

  /* shuffle UIDs for permissions - we expect to be running SUID-root */
  if (setuid(geteuid()) != 0) {
    fprintf(stderr, "Failed to properly set UID, exiting...\n");
    exit(1);
  }

  /* open a pipe to and then feed the standard Unix passwd program, slowly */
  if (isatty(0))
    fprintf(mystderr, "Changing Unix password...\n");
  strcpy(cmd, PASSWD);
  strcat(cmd, " ");
  strcat(cmd, pwentry->pw_name);
  if ((cmdpipe = popen(cmd, "w")) == NULL) {
    fprintf(mystderr, "Failed to open pipe to '%s', exiting...\n", cmd);
    exit(1);
  }
  sleep(3);
  fprintf(cmdpipe, "%s\n", new); fflush(cmdpipe); sleep(2);
  fprintf(cmdpipe, "%s\n", new); fflush(cmdpipe); sleep(2);
  if ((status = pclose(cmdpipe)) != 0) {
    fprintf(mystderr, "Program '%s' returned error code %d, exiting...\n",
	    cmd, status);
    exit(1);
  }
  if (isatty(0))
    fprintf(mystderr, "\tSuccessfully changed Unix password.\n");

  /* now change the SMB password */
  if (isatty(0))
    fprintf(mystderr, "Changing SMB/Windows password...\n");
  strcpy(cmd, SMBPASSWD);
  strcat(cmd, " ");
  strcat(cmd, pwentry->pw_name);
  if ((cmdpipe = popen(cmd, "w")) == NULL) {
    fprintf(mystderr, "Failed to open pipe to '%s', exiting...\n", cmd);
    exit(1);
  }
  sleep(3);
  fprintf(cmdpipe, "%s\n", new); fflush(cmdpipe); sleep(2);
  fprintf(cmdpipe, "%s\n", new); fflush(cmdpipe); sleep(2);
  if ((status = pclose(cmdpipe)) != 0) {
    fprintf(mystderr, "Program '%s' returned error code %d, exiting...\n",
	    cmd, status);
    exit(1);
  }
  if (isatty(0))
    fprintf(mystderr, "\tSuccessfully changed SMB/Windows password.\n");

  exit(0);
}

/* end of program */
