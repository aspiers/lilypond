/*
  all-font-metrics.cc -- implement All_font_metrics

  source file of the GNU LilyPond music typesetter

  (c) 1999--2006 Han-Wen Nienhuys <hanwen@xs4all.nl>
*/

#include "all-font-metrics.hh"

#include "international.hh"
#include "main.hh"
#include "open-type-font.hh"
#include "pango-font.hh"
#include "scm-hash.hh"
#include "warn.hh"


Index_to_charcode_map const *
All_font_metrics::get_index_to_charcode_map (string filename, FT_Face face)
{
  if (filename_charcode_maps_map_.find (filename)
      == filename_charcode_maps_map_.end ())
    filename_charcode_maps_map_[filename] = make_index_to_charcode_map (face);

  return &filename_charcode_maps_map_[filename];
}


All_font_metrics::All_font_metrics (string path)
{
  otf_dict_ = new Scheme_hash_table;

#if HAVE_PANGO_FT2
  PangoFontMap *pfm = pango_ft2_font_map_new ();

  pango_ft2_fontmap_
    = G_TYPE_CHECK_INSTANCE_CAST (pfm,
				  PANGO_TYPE_FT2_FONT_MAP,
				  PangoFT2FontMap);
  pango_dpi_ = 1200;
  pango_ft2_font_map_set_resolution (pango_ft2_fontmap_,
				     pango_dpi_, pango_dpi_);

  pango_dict_ = new Scheme_hash_table;
#endif

  search_path_.parse_path (path);
}

All_font_metrics::~All_font_metrics ()
{
  otf_dict_->unprotect ();

#if HAVE_PANGO_FT2
  pango_dict_->unprotect ();
  g_object_unref (pango_ft2_fontmap_);
#endif
}

All_font_metrics::All_font_metrics (All_font_metrics const &)
{
}

#if HAVE_PANGO_FT2

Pango_font *
All_font_metrics::find_pango_font (PangoFontDescription const *description,
				   Real output_scale
				   )
{
  gchar *pango_fn = pango_font_description_to_filename (description);
  SCM key = ly_symbol2scm (pango_fn);

  SCM val;
  if (!pango_dict_->try_retrieve (key, &val))
    {
      if (be_verbose_global)
	progress_indication ("[" + string (pango_fn));

      Pango_font *pf = new Pango_font (pango_ft2_fontmap_,
				       description,
				       output_scale
				       );
      
      val = pf->self_scm ();
      pango_dict_->set (key, val);
      pf->unprotect ();

      if (be_verbose_global)
	progress_indication ("]");

      pf->description_ = scm_cons (SCM_BOOL_F,
				   scm_from_double (1.0));
    }
  g_free (pango_fn);
  return dynamic_cast<Pango_font *> (unsmob_metrics (val));
}

#endif

string
kpathsea_find_file (string name, string ext)
{
  name += "." + ext;
  string path = global_path.find (name);
  if (path.length () > 0)
    return path;

  static SCM proc;
  if (!proc)
    {
      SCM module = scm_c_resolve_module ("scm kpathsea");
      proc = scm_c_module_lookup (module, "ly:kpathsea-find-file");
      proc = scm_variable_ref (proc);
    }

  if (ly_is_procedure (proc))
    {
      SCM kp_result = scm_call_1 (proc, scm_makfrom0str (name.c_str ()));
      if (scm_is_string (kp_result))
	return ly_scm2string (kp_result);
    }

  return "";
}

Open_type_font *
All_font_metrics::find_otf (string name)
{
  SCM sname = ly_symbol2scm (name.c_str ());
  SCM name_string = scm_makfrom0str (name.c_str ());
  SCM val;
  if (!otf_dict_->try_retrieve (sname, &val))
    {
      string file_name;

      if (file_name.empty ())
	file_name = search_path_.find (name + ".otf");
      if (file_name.empty ())
	return 0;

      if (be_verbose_global)
	progress_indication ("[" + file_name);

      val = Open_type_font::make_otf (file_name);

      if (be_verbose_global)
	progress_indication ("]");

      unsmob_metrics (val)->file_name_ = file_name;
      unsmob_metrics (val)->description_ = scm_cons (name_string,
						     scm_from_double (1.0));
      otf_dict_->set (sname, val);
      unsmob_metrics (val)->unprotect ();
    }

  return dynamic_cast<Open_type_font *> (unsmob_metrics (val));
}

Font_metric *
All_font_metrics::find_font (string name)
{
  Font_metric *f = find_otf (name);

  if (!f)
    {
      error (_f ("can't find font: `%s'", name.c_str ()));
    }

  return f;
}

All_font_metrics *all_fonts_global;

LY_DEFINE (ly_reset_all_fonts, "ly:reset-all-fonts", 0, 0, 0,
	   (),
	   "Forget all about previously loaded fonts. ")
{
  delete all_fonts_global;
  all_fonts_global = new All_font_metrics (global_path.to_string ());

  return SCM_UNSPECIFIED;
}


LY_DEFINE (ly_font_load, "ly:font-load", 1, 0, 0,
	   (SCM name),
	   "Load the font @var{name}. ")
{
  SCM_ASSERT_TYPE (scm_is_string (name), name, SCM_ARG1, __FUNCTION__, "string");

  Font_metric *fm = all_fonts_global->find_font (ly_scm2string (name));

  return fm->self_scm ();
}


