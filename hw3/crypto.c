#include "crypto.h"

void __to_hex(char *dst, char *src, size_t src_size)
{
	int x;

	for (x = 0; x < src_size; x++)
		sprintf(&dst[x * 2], "%.2x", (unsigned char)src[x]);
	dst[x*2] = 0;
}

static int init_desc(char *hash, struct hash_desc *desc)
{
	int rc;

	desc->tfm = crypto_alloc_hash(hash, 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(desc->tfm)) {
		pr_info("failed to load %s transform: %ld\n",
				hash, PTR_ERR(desc->tfm));
		rc = PTR_ERR(desc->tfm);
		return rc;
	}
	desc->flags = 0;
	rc = crypto_hash_init(desc);
	if (rc)
		crypto_free_hash(desc->tfm);
	return rc;
}

int calc_hash(char *hashkey, struct file *file, char **hash)
{
	struct hash_desc desc;
	struct scatterlist sg[1];
	loff_t i_size, offset = 0;
	int rc, digestsize;
	char *rbuf, *digest;

	*hash = NULL;

	rc = init_desc(hashkey, &desc);
	if (rc != 0)
		return rc;

	rbuf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!rbuf) {
		rc = -ENOMEM;
		goto out;
	}

	digestsize = crypto_hash_digestsize(desc.tfm);
	*hash = kzalloc(digestsize * 2 + 1, GFP_KERNEL);
	if (!*hash) {
		rc = -ENOMEM;
		goto out;
	}
	digest = kzalloc(digestsize + 1, GFP_KERNEL);
	if (!digest) {
		rc = -ENOMEM;
		kfree(*hash);
		*hash = NULL;
		goto out;
	}
	i_size = i_size_read(file->f_dentry->d_inode);
	while (offset < i_size) {
		int rbuf_len;

		rbuf_len = kernel_read(file, offset, rbuf, PAGE_SIZE);
		if (rbuf_len < 0) {
			rc = rbuf_len;
			break;
		}
		if (rbuf_len == 0)
			break;
		offset += rbuf_len;
		sg_init_one(sg, rbuf, rbuf_len);

		rc = crypto_hash_update(&desc, sg, rbuf_len);
		if (rc)
			break;
	}
	kfree(rbuf);
	if (!rc)
		rc = crypto_hash_final(&desc, digest);
	if (rc) {
		kfree(*hash);
		*hash = NULL;
	} else
		__to_hex(*hash, digest, digestsize);
	kfree(digest);
out:
	crypto_free_hash(desc.tfm);
	return rc;
}

int encrpt_decrypt_file(struct file *infile, struct file *outfile, char *algo, char *key, int keysize, char *iv, int enc)
{
	struct crypto_blkcipher *tfm;
	struct blkcipher_desc desc;
	struct scatterlist sg[2];
	loff_t i_size, offset = 0;
	int rc;
	char *input, *outbuf;


	tfm = crypto_alloc_blkcipher(algo, 0, CRYPTO_ALG_ASYNC);

	if (IS_ERR(tfm)) {
		printk("failed to load transform for %s \n", algo);
		rc = PTR_ERR(tfm);
		return rc;
	}
	desc.tfm = tfm;
	desc.flags = 0;
	rc = crypto_blkcipher_setkey(tfm, key, keysize);

	if (rc) {
		printk(KERN_ERR  "setkey() failed flags=%x\n", tfm->base.crt_flags);
		goto out;
	}

	input = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!input) {
		rc = -ENOMEM;
		printk(KERN_ERR  "kmalloc(input) failed\n");
		goto out;
	}

	outbuf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!outbuf) {
		printk(KERN_ERR  "kmalloc(outbuf) failed\n");
		rc = -ENOMEM;
		kfree(input);
		goto out;
	}

	//crypto_blkcipher_set_iv(tfm, iv, crypto_blkcipher_ivsize(tfm));
	i_size = i_size_read(infile->f_dentry->d_inode);
	while (offset < i_size) {
		int rbuf_len, wbuf_len;

		rbuf_len = kernel_read(infile, offset, input, PAGE_SIZE);
		if (rbuf_len < 0) {
			rc = rbuf_len;
			break;
		}
		if (rbuf_len == 0)
			break;

		sg_init_one(sg, input, PAGE_SIZE);
		sg_init_one(sg + 1, outbuf, PAGE_SIZE);

		if (enc) {
			rc = crypto_blkcipher_encrypt(&desc, &sg[1], &sg[0], PAGE_SIZE);
		}
		else {
			printk("\ndecrypt\n");
			msleep(5000);
			rc = crypto_blkcipher_decrypt(&desc, &sg[1], &sg[0], PAGE_SIZE);
		}

		printk("\nAfter Operation\n");
		msleep(2000);
		if (rc)
			break;
		wbuf_len = myWrite(outfile, outbuf, rbuf_len, offset);
		if (wbuf_len < 0) {
			rc = wbuf_len;
			printk(KERN_ERR  "writing failed, error code:%d\n", rc);
			break;
		}

		offset += rbuf_len;
	}
	if (rc) {
		printk(KERN_ERR  "encryption failed, flags=0x%x, error code:%d\n", tfm->base.crt_flags, rc);
	}

	kfree(outbuf);
	kfree(input);

out:
	crypto_free_blkcipher(tfm);
	return rc;
}
