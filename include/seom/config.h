
#ifndef __SEOM_CONFIG_H__
#define __SEOM_CONFIG_H__

typedef struct seomConfig {
	uint32_t insets[4];
	double interval;
	char scale;
	
	struct sockaddr_in addr;
} seomConfig;

seomConfig *seomConfigCreate(const char *ns);
void seomConfigDestroy(seomConfig *config);

#endif /* __SEOM_CONFIG_H__ */
