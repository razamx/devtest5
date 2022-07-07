## Logging support

Intel® TCC Tools uses logging internally. To enable log output you can set TCC_LOG_LEVEL environment variable.

 Possible values |
-----------------|
 TRACE |
 DEBUG |
 INFO |
 WARNING |
 ERROR |
 FATAL |
 NONE |

 To redirect log output to file you can set TCC_LOG_TO_FILE environment variable.
 ~~~
 TCC_LOG_TO_FILE=somefile.log TCC_LOG_LEVEL=DEBUG ./sample
 ~~~
