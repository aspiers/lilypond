
\version "2.6.0"

\header {
    texidoc = "Accidentals do not influence the amount of stretchable space.
The accidental does add a little non-stretchable space. 
"
}

\score {
      \relative c'' \context GrandStaff {
	#(set-accidental-style 'piano-cautionary)
	\time 2/4 
	d16 d d d d d cis d dis dis dis dis 
	
     }

     %% not raggedright!!
     \layout {  linewidth = 18.\cm
     }
}
