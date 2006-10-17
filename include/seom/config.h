
#ifndef __SEOM_CONFIG_H__
#define __SEOM_CONFIG_H__

typedef struct seomConfig {
} seomConfig;

void seomConfigServer(const char *ns, char server[256]);
void seomConfigInterval(const char *ns, double *v);
void seomConfigScale(const char *ns, char scale[64]);
void seomConfigInsets(const char *ns, uint32_t v[4]);

#endif /* __SEOM_CONFIG_H__ */
