;
; Opencog cogserver module
;

(define-module (opencog cogserver))

(use-modules (opencog))
(use-modules (opencog cs-config))

; Path to libguile-cogserver.so is set up in the cs-config module.
(load-extension
	(string-append opencog-ext-path-cogserver "libguile-cogserver")
	"opencog_cogserver_init")

; config path name is optional.
(define* (start-cogserver #:optional (config-path "")
                          #:key (port 17001)
                                (logfile "/tmp/cogserver.log")
                                (loglevel "info"))
"
  start-cogserver
  start-cogserver #:port 17001
  start-cogserver #:logfile \"/tmp/cogserver.log\"
  start-cogserver #:loglevel \"info\"
  start-cogserver [config-file]

  Start the cogserver, optionally specifying a port, logfile and
  loglevel (or any combination of these). If any are missing, default
  values will be used. The defaults are as shown. Available loglevels
  are \"error\", \"warning\", \"info\", \"debug\" and \"fine\". Each level
  places more detailed information into the log.

  Alternately, a config file can be specified. If a config file is
  given, then the port, logfile and loglevel will be obtained from the
  config file, and the other optional arguments will be ignored. The
  use of a config file is deprecated.

  To stop the cogserver, just say stop-cogserver.
"
	(c-start-cogserver (cog-atomspace) port logfile loglevel config-path)
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
