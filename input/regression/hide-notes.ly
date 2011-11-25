
\version "2.14.0"

\header {
  texidoc = "Notes can be hidden via \hideNote(s)."
}

\layout { ragged-right= ##t }

\relative c'
\context Voice {
  c4 d
  \hideNotes
  e4 f
  \unHideNotes
  g a
  \hideNote
  b
  c
}

