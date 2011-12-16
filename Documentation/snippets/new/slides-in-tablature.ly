\version "2.14.0"

\header {
  lsrtags = "fretted-strings"
  texidoc = "
Slides can be typeset in both @code{Staff} and @code{TabStaff} contexts:
"
  doctitle = "Slides in tablature"
}

slides = {
  c'8\3(\glissando d'8\3)
  c'8\3\glissando d'8\3
  \hideNote
  \grace { g16\3\glissando }
  c'4\3
  \afterGrace d'4\3\glissando {
  \stemDown \hideNote
  g16\3 }
}

\score {
  <<
    \new Staff { \clef "treble_8" \slides }
    \new TabStaff { \slides }
  >>
  \layout {
    \context {
      \Score
      \override Glissando #'minimum-length = #4
      \override Glissando #'springs-and-rods =
                          #ly:spanner::set-spacing-rods
      \override Glissando #'thickness = #2
    }
  }
}
