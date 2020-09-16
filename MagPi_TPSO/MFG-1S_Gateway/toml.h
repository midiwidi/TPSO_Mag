#ifndef TOML_H_
#define TOML_H_

#include "config.h"

#define DEFAULT_CONFIG_FILE		"MFG-1S_Gateway.toml"

int read_config(char* conf_file, struct config *cfg);

#endif /* TOML_H_ */
