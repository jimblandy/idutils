;;; put this in your GnuEmacs startup file  '~/.emacs' .
;;; or autoload it from some other file. -wsr

(require 'symfunc)

(defun gid (command)
  "Run gid, with user-specified args, and collect output in a buffer.
While gid runs asynchronously, you can use the \\[next-error] command
to find the text that gid hits refer to."
  (interactive (list (read-input "Run gid (with args): "
				 (symbol-around-point))))
  (require 'compile)
  (setq command (concat "gid " command))
  (compile1 command "No more gid hits" command))

(defun aid (command)
  "Run aid, with user-specified args, and collect output in a buffer."
  (interactive (list (read-input "Run aid (with args): "
				 (symbol-around-point))))
  (require 'compile)
  (setq command (concat "aid " command))
  (compile1 command "No aid hits" command))
