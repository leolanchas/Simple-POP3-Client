POP3 Bash Client
----------------

This is a VERY, VERY RAW implementation of a pop3 client just to experiment with sockets in C.

It was coded using Netbeans IDE in Linux.

Important: DO NOT USE WITH YOUR PERSONAL E-MAIL ACCOUNT. NOTHING is encrypted and read
mail are stored in the MAILS folder in PLAIN TEXT.

Tests were done with a test Yahoo email account, hence you will find its server's IPs in client.txt.
You can change them if you want to use another email service.

Instructions:
------------

At readFile.c, at lines 290 and 361 you must replace "YOURUSER@DOMAIN.COM" with your email account. For example
JohnDoe@yahoo.com.

When you have finished composing your email, you must add at the end "__MF__" to indicate that to the program.
It's terrible, I know but that was the way i came up with to detect the end of the message within a bash console.