
// configuration file
extern File cfg_file;
extern IniFile cfgfile;

extern struct config cfg;

static inline void set_config_file(File f)
{
  cfg_file = f;
  cfgfile.setFile(f);
}
