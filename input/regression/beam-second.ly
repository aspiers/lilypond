
\version "2.1.28"
\header{
  texidoc="
Engraving second intervals is tricky.  We used to have problems with seconds 
being too steep, or getting too long stems.  In a file like this, showing
seconds, you'll spot something fishy very quickly.
 
" }
\score{
    \notes\relative c''{
	\stemUp
	 b8[ c]
	 b16[ c]
	 a'[ b]
    }
    \paper{
	raggedright = ##t
    }
}
