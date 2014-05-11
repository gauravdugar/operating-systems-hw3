#ifndef CRYPTO_H

#define CRYPTO_H

#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/delay.h>

#include "file.h"

extern int calc_hash(char *hash, struct file *file, char **);
extern int encrpt_decrypt_file(struct file *infile, struct file *outfile, char *algo, char *key, int keysize, char *iv, int enc);

#endif
