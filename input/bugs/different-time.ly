%
% irregular spacing.

\score{
    \context PianoStaff <
        \context Staff = upper \notes\relative c''{
	    \property Staff.timeSignatureFraction = #'(3 . 4)
	    c4 c c
	    c2.
	  	    
        }
        \context Staff = lower \notes\relative c' {
	    \property Staff.timeSignatureFraction = #'(4 . 4)
	    c4 c c c
	    c1
        }
    > 
    \paper{
    	linewidth=-1.0;
	\translator {
	    \ScoreContext
	    \remove Timing_engraver;
	}
	\translator {
	    \StaffContext
	    \consists Timing_engraver;
	}
    }
}
