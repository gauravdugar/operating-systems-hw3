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
		int read_len;

		read_len = kernel_read(file, offset, rbuf, PAGE_SIZE);
		if (read_len < 0) {
			rc = read_len;
			break;
		}
		if (read_len == 0)
			break;
		offset += read_len;
		sg_init_one(sg, rbuf, read_len);

		rc = crypto_hash_update(&desc, sg, read_len);
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

int encrypt_decrypt_file(struct file *infile, struct file *outfile, char *algo,
		char *key, int keysize, char *iv, int enc)
{
	struct crypto_blkcipher *tfm;
	struct blkcipher_desc desc;
	struct scatterlist sg[2];
	loff_t i_size, offset = 0;
	int rc = 0;
	int read_len = 0;
	int write_len = 0;
	char *input, *outbuf;
	int block = 0;

	tfm = crypto_alloc_blkcipher(algo, 0, CRYPTO_ALG_ASYNC);

	if (IS_ERR(tfm)) {
		printk(KERN_ERR "failed to load transform for %s\n", algo);
		rc = PTR_ERR(tfm);
		return rc;
	}

	desc.tfm = tfm;
	desc.flags = 0;

	printk(KERN_INFO "\nKey = %s, KeySize = %d\n", key, keysize);
	msleep(1000);
	rc = crypto_blkcipher_setkey(tfm, key, keysize);
	block = crypto_tfm_alg_blocksize(&tfm->base);
	if (rc) {
		printk(KERN_ERR "setkey() failed flags=%x\n",
			tfm->base.crt_flags);
		goto out;
	}

	input = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!input) {
		rc = -ENOMEM;
		printk(KERN_ERR "kzalloc(input) failed\n");
		goto out;
	}

	outbuf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!outbuf) {
		printk(KERN_ERR "kzalloc(outbuf) failed\n");
		rc = -ENOMEM;
		kfree(input);
		goto out;
	}

	i_size = i_size_read(infile->f_dentry->d_inode);
	while (offset < i_size) {
		read_len = kernel_read(infile, offset, input, PAGE_SIZE);
		if (read_len < 0) {
			rc = read_len;
			break;
		}

		if (enc && read_len < PAGE_SIZE) {
			if (read_len % block != 0) {
				int pad = block - read_len % block;
				write_len = read_len + pad;
				memset(input + read_len, pad, pad);
			} else
				write_len = read_len;
		}

		sg_init_one(sg, input, PAGE_SIZE);
		sg_init_one(sg + 1, outbuf, PAGE_SIZE);

		if (enc)
			rc = crypto_blkcipher_encrypt(&desc, &sg[1],
					&sg[0], PAGE_SIZE);
		else
			rc = crypto_blkcipher_decrypt(&desc, &sg[1],
					&sg[0], PAGE_SIZE);

		if (rc)
			break;

		if (unlikely(!enc &&  offset + read_len >= i_size)) {
			int pad = *(outbuf + read_len - 1);
			write_len = read_len - pad;
			if (pad != 0) {
				int i = 0;
				for (i = 1; i < pad; i++) {
					if (*(outbuf + read_len - i - 1)
							!= pad) {
						write_len = read_len;
						break;
					}
				}
			}
		}

		write_len = myWrite(outfile, outbuf, write_len, offset);
		if (write_len < 0) {
			rc = write_len;
			printk(KERN_ERR "writing failed, error code:%d\n", rc);
			break;
		}

		offset += read_len;
	}

	if (rc)
		printk(KERN_ERR "encrypt failed, flags=0x%x, error code:%d\n",
			tfm->base.crt_flags, rc);

	kfree(outbuf);
	kfree(input);

out:
	crypto_free_blkcipher(tfm);
	return rc;
}
