\version "2.1.28"

\header {

texidoc = "skipTypesetting doesn't affect bar checks."

}

\score { \notes {
	\set Score.skipTypesetting = ##t
	c4 c4
	|
	c4 c4 }}
