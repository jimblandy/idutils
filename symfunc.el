;; This file provides functions for symbols, that is, things consisting only
;; of characters matching the regular expression \(\w\|\s_\).  The functions
;; are similar to those provided for words (e.g., symbol-around-point is
;; just like word-around-point).

(provide 'symfunc)

(defvar symbol-char-re "\\(\\w\\|\\s_\\)"
"The regular expression that matches a character belonging to a symbol.")

(defun symbol-around-point ()
  "return the symbol around the point as a string"
  (save-excursion
    (let (beg)
      (if (not (at-beginning-of-symbol))
	  (forward-symbol -1))
      (setq beg (point))
      (forward-symbol 1)
      (buffer-substring beg (point))
    )
  )
)

(defun at-beginning-of-symbol ()
"Return t if point is currently positioned at the beginning of
a symbol."
   (and
      (looking-at symbol-char-re)
      (not (looking-back symbol-char-re))
   )
)

(defun forward-symbol (arg)
"Move point forward ARG symbols (backward if ARG is negative).
Normally returns t.
If an edge of the buffer is reached, point is left there
and nil is returned.
It is faster to call backward-symbol than to call forward-symbol
with a negative argument."
   (interactive "p")
   (if (null arg)
      (setq arg 1)
   )
   (if (< arg 0)
      (backward-symbol (- arg))
      (progn
         (while (> arg 0)
            (condition-case ()
               (progn
                  (while (not (looking-at symbol-char-re))
                     (forward-char 1)
                  )
                  (while (looking-at symbol-char-re)
                     (forward-char 1)
                  )
                  t
               )
               (error nil)          ;; Return nil if error
            )
            (setq arg (1- arg))
         )
      )
   )
)

(defun backward-symbol (arg)
"Move backward until encountering the end of a symbol.
With argument, do this that many times.
In programs, it is faster to call forward-symbol
than to call backward-symbol with a negative arg."
   (interactive "p")
   (if (null arg)
      (setq arg 1)
   )
   (if (< arg 0)
      (forward-symbol (- arg))
      (progn
         (while (> arg 0)
            (condition-case ()
               (progn
                  (while (not (looking-back symbol-char-re))
                     (forward-char -1)
                  )
                  (while (looking-back symbol-char-re)
                     (forward-char -1)
                  )
                  t
               )
               (error nil)          ;; Return nil if error
            )
            (setq arg (1- arg))
         )
      )
   )
)

;; Additional word-oriented functions.

(defun word-around-point ()
  "return the word around the point as a string"
  (save-excursion
    (let (beg)
      (if (not (looking-at "\\<"))
	  (forward-word -1))
      (setq beg (point))
      (forward-word 1)
      (buffer-substring beg (point)))))

;; The looking-back function used to exist in Emacs distribution, but
;; it disappeared in 18.52.

(defun looking-back (str)
  "returns t if looking back reg-exp STR before point."
  (and
     (save-excursion (re-search-backward str nil t))
     (= (point) (match-end 0))
  )
)
