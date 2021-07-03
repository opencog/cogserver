;
; Opencog cogserver module
;

(define-module (opencog cogserver))

(use-modules (opencog))
(use-modules (opencog cs-config))
(use-modules (opencog logger))

; Path to libguile-cogserver.so is set up in the cs-config module.
(load-extension
	(string-append opencog-ext-path-cogserver "libguile-cogserver")
	"opencog_cogserver_init")

; config path name is optional.
(define* (start-cogserver #:optional (config-path "")
                          #:key (port 17001)
                                (logfile   "/tmp/cogserver.log")
                                (prompt    "[0;32mopencog[1;32m> [0m")
                                (scmprompt "[0;34mguile[1;34m> [0m"))
"
  start-cogserver
  start-cogserver #:port 17001
  start-cogserver #:logfile \"/tmp/cogserver.log\"
  start-cogserver #:prompt \"[0;32mopencog[1;32m> [0m\"
  start-cogserver #:prompt \"\\x1b[0;32mopencog\\x1b[1;32m> \\x1b[0m\"
  start-cogserver #:scmprompt \"[0;34mguile[1;34m> [0m\"
  start-cogserver #:scmprompt \"\\x1b[0;34mguile\\x1b[1;34m> \\x1b[0m\"
  start-cogserver [config-file]

  Start the cogserver, optionally specifying a port, logfile, telnet
  prompt and telnet scheme prompt (or any combination of these). If any
  are missing, default values will be used. The defaults are as shown.
  Additinal logging options can be found in the (opencog logger) module.

  The prompts may be ANSI colorized prompts. The ANSI escape sequence
  uses the ESC char, which is written as \\x1b in guile.

  Alternately, a config file can be specified. If a config file is
  given, as well as the abbove paramters, then the above parameters take
  precedence.  The use of a config file is deprecated.

  To stop the cogserver, just say stop-cogserver.
"
	(cog-logger-set-filename! logfile)
	(c-start-cogserver (cog-atomspace) port prompt scmprompt config-path)
)

; To stop the repl server..
(use-modules (system repl server))

; Similar to above
(define (stop-cogserver)
"
  stop-cogserver

  Stop the cogserver.

  See also: start-cogserver
"
	; The start-cogserver also starts a repl shell on port 18001
	; so we stop that, here ...
	(stop-server-and-clients!)
	(c-stop-cogserver)
)

(export start-cogserver)
(export stop-cogserver)
