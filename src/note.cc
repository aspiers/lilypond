/*
  could use cleanup
 */
#include <ctype.h>
#include "lexer.hh"
#include "string.hh"
#include "real.hh"
#include "debug.hh"
#include "musicalrequest.hh"
#include "commandrequest.hh"
#include "voice.hh"
#include "notename.hh"
#include "identparent.hh"
#include "varray.hh"
#include "text-def.hh"
#include "parseconstruct.hh"
#include "input-music.hh"
#include "voice-element.hh"

int default_duration = 4, default_dots=0, default_octave=0;
int default_plet_type = 1, default_plet_dur = 1;
String textstyle="roman";		// in lexer?

bool last_duration_mode = false;

void
set_duration_mode(String s)
{
    s = s.upper_str();
    last_duration_mode = (s== "LAST");
}

void
last_duration(int n)
{
    if (last_duration_mode)
	default_duration = n;
}

/* triplet is '2/3' */
void 
set_plet(int num,int den)
{
    assert(num >0&& den>0);
    default_plet_dur = num;
    default_plet_type = den;
}

Text_def*
get_text(String s) return t;
{
    t= new Text_def;
    t->text_str_= s;
    t->style_str_ = textstyle;
    t->defined_ch_c_l_ = defined_ch_c_l;
    return t;
}

void
set_text_style(String s)
{
    textstyle = s;
}

void
parse_octave (const char *a, int &j, int &oct)
{    
    while (1) 
    {	
	if (a[j] == '\'')
	    oct ++;
	else if (a[j] == '`')
	    oct --;
	else
	    break;
	j++;
    }
}

void 
parse_pitch( const char *a, Melodic_req* mel_l)
{
    int j=0;

    // octave
    mel_l->octave_i_ = default_octave;
    parse_octave(a,j,mel_l->octave_i_);

    // accidental
    mel_l->forceacc_b_ = false;
    
    if (a[j] == '!'){
	mel_l->forceacc_b_ = true;
	j++;
    }
}

Voice_element *
get_note_element(String pitch, int * notename, int * duration )
{
    Voice_element*v = new Voice_element;
    v->defined_ch_c_l_ = defined_ch_c_l;

    
    int dur = duration[0];
    int dots = duration[1];

    if (dur >= 2) {
	Stem_req * stem_req_p = new Stem_req(dur,dots);
	stem_req_p->plet_factor = Moment(default_plet_dur, default_plet_type);
	stem_req_p->defined_ch_c_l_ = defined_ch_c_l;
	v->add(stem_req_p);
    }
    
    if ( !defined_ch_c_l )
        defined_ch_c_l = lexer->here_ch_c_l();

    Note_req * rq = new Note_req;
    rq->notename_i_ =notename[0];
    rq->accidental_i_ = notename[1];
    parse_pitch(pitch, rq);
    rq->octave_i_ += notename[2];

    rq->balltype = dur;
    rq->dots = dots;
    rq->plet_factor = Moment(default_plet_dur, default_plet_type);
    rq->defined_ch_c_l_ = defined_ch_c_l;

    v->add(rq);

    return v;
}

Voice_element*
get_word_element(Text_def* tdef_p, int* duration)
{
    Voice_element* velt_p = new Voice_element;
    velt_p->defined_ch_c_l_ = defined_ch_c_l;
    
    int dur = duration[0];
    int dots=duration[1];
    
    tdef_p->defined_ch_c_l_ = defined_ch_c_l;

    Lyric_req* lreq_p = new Lyric_req(tdef_p);

    lreq_p->balltype = dur;
    lreq_p->dots = dots;
    lreq_p->plet_factor = Moment(default_plet_dur, default_plet_type);
    lreq_p->print();
    lreq_p->defined_ch_c_l_ = defined_ch_c_l;

    velt_p->add(lreq_p);

    return velt_p;
}

Voice_element *
get_rest_element(String,  int * duration )
{    
    Voice_element* velt_p = new Voice_element;
    velt_p->defined_ch_c_l_ = defined_ch_c_l;

    Rest_req * rest_req_p = new Rest_req;
    rest_req_p->plet_factor = Moment(default_plet_dur, default_plet_type);
    rest_req_p->balltype = duration[0];
    rest_req_p->dots = duration[1];    
    rest_req_p->print();
    rest_req_p->defined_ch_c_l_ = defined_ch_c_l;

    velt_p->add(rest_req_p);

    return velt_p;
}

void
get_default_duration(int *p)
{
    *p++ = default_duration;
    *p = default_dots;
}

void
set_default_duration(int *p)
{
     default_duration = *p++;
     default_dots = *p++;
}


void
set_default_octave(String d)
{
    int i=0;
    default_octave=0;
    parse_octave(d, i, default_octave);
}

Request*
get_plet_request( char c, int dur_i, int type_i )
{
    Plet_req* plet_req_p = new Plet_req;
    plet_req_p->dur_i_ = dur_i;
    plet_req_p->type_i_ = type_i;
    plet_req_p->type_c_ = c;
    return plet_req_p;
}

Request*
get_request(char c)
{
    Request* req_p=0;
    switch (c) {
    case '|':
	req_p = new Barcheck_req;
	break;

    case '[':
    case ']':
    {
	Beam_req*b = new Beam_req;
	if (default_plet_type != 1)
	    b->nplet = default_plet_type;
	req_p = b;
    }
	break;


    case ')':
    case '(':
	req_p = new Slur_req;
	break;
    default:
	assert(false);
	break;
    }
    
    switch (c) {
    case '(':
    case '[':
	req_p->span()->spantype = Span_req::START;
	break;
    case ')':
    case ']':
	req_p->span()->spantype = Span_req::STOP;
	break;
	
    default:
	break;
    }

    req_p->defined_ch_c_l_ = req_defined_ch_c_l;
    return req_p;
}

void
add_requests(Voice_element *v, Array<Request*> &req)
{
    for (int i = 0; i < req.size(); i++) {
	v->add(req[i]);
    }
    req.set_size(0);
}

Script_def*
get_scriptdef(char c)
{
    String s;
    switch (c) {
    case '^' : s = "marcato";
	break;
    case  '+' : s = "stopped";
	break;
    case '-' : s = "tenuto";
	break;
    case  '|':  s = "staccatissimo";
	break;
    case  'o' : s = "";
	break;
    case '>' : s = "accent";
	break;
    case  'v' : s = ""; 
	break;
    case  '.' : s = "staccato";
	break;
    }
    return lexer->lookup_identifier(s)->script(1);
}

Request*
get_script_req(int d , Script_def*def)
{
    Script_req* script_req_p = new Script_req(d, def);
    return script_req_p;
}

Request*
get_text_req(int d , Text_def*def)
{
    Text_req* text_req_p = new Text_req(d, def);
    return text_req_p;
}

Request*
get_stemdir_req(int d)
{
    Group_feature_req * gfreq_p = new Group_feature_req;
    gfreq_p->stemdir_i_ =d; 
    return gfreq_p;
}

Request*
get_grouping_req(Array<int> i_arr)
{
    Measure_grouping_req * mr_p = new Measure_grouping_req;
    for (int i=0; i <i_arr.size(); ) {
	mr_p->beat_i_arr_.push(i_arr[i++]);
	mr_p->elt_length_arr_.push(Moment(1, i_arr[i++]));
    }
    return mr_p;
}
