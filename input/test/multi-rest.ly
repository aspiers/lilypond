

voiceOne =  \notes\transpose c''{  \stemUp
	R1 * 2 | f'4-. r r2 | R1 * 3 |
	f'4-. r r2 | R1 * 3 |
	es'4-. r r2 | r1 |
	\property Score.skipBars = ##t6
	g4-. r r2 | g4-. r r2 |
	R1 * 5 |
	r2 r4 g4-. |
	}

voiceTwo =  \notes
	{ \transpose c' { \stemDown
	R1 * 2 | f'4-. r r2 | R1 * 3 |
	f'4-. r r2 | R1 * 3 |
	es'4-. r r2 | r1 |
	\property Score.skipBars = ##t6
	g4-. r r2 | g4-. r r2 |
	R1 * 5 |
	r2 r4 g4-. |

	}}

\score {  \notes
	\context Staff <
		\voiceTwo
		\voiceOne
		>

 }

