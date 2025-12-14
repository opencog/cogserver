#! /usr/bin/env -S guile -s
!#
;
; cog-server-test.scm -- Test that the cogserver module loads and works
;
(use-modules (opencog))
(use-modules (opencog test-runner))
(use-modules (opencog cogserver))
(use-modules (ice-9 rdelim))

(opencog-test-runner)
(define tname "cog-server-test")
(test-begin tname)

(test-assert "CogServerNode-type-exists"
   (defined? 'CogServerNode))

(test-assert "CogServerNode-create"
   (let ((csn (CogServerNode "test-server")))
      (cog-atom? csn)))

;; Helper function to test if a port can be connected to
(define (can-connect-to-port? port)
   (catch #t
      (lambda ()
         (let ((sock (socket AF_INET SOCK_STREAM 0)))
            (connect sock AF_INET INADDR_LOOPBACK port)
            (close-port sock)
            #t))
      (lambda (key . args)
         #f)))

;; Test starting the cogserver, connecting to ports, and stopping
;; Use non-standard ports to avoid conflicts with other running servers
(define test-telnet-port 17101)
(define test-web-port 18180)

(test-assert "start-stop-cogserver"
   (begin
      ;; Start the cogserver with test ports
      (start-cogserver #:port test-telnet-port
                       #:web test-web-port
                       #:mcp 0)

      ;; Test connectivity
      (let ((telnet-ok (can-connect-to-port? test-telnet-port))
            (web-ok (can-connect-to-port? test-web-port)))

         (stop-cogserver)

         ;; Return success if both ports were connectable
         (and telnet-ok web-ok))))

;; Test that ports are no longer listening after stop
(test-assert "ports-closed-after-stop"
   (begin
      ;; Is this needed? Want full tcpip shutdown.
      (usleep 200000)
      ;; Both ports should now be closed
      (not (or (can-connect-to-port? test-telnet-port)
               (can-connect-to-port? test-web-port)))))

(test-end tname)

(opencog-test-end)
