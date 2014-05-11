#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/export.h>

#define NETLINK_USER 31
struct sock *nl_sk = NULL;

void send_msg(int pid, char *msg)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb_out;
	int msg_size;
	int res;

	msg_size=strlen(msg);
	skb_out = nlmsg_new(msg_size,0);

	if(!skb_out)
	{
		printk(KERN_ERR "Failed to allocate new skb\n");
		return;
	}
	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
	NETLINK_CB(skb_out).dst_group = 0;
	strncpy(nlmsg_data(nlh), msg, msg_size);

	//msleep(5000);

	res = nlmsg_unicast(nl_sk,skb_out,pid);

	printk("\n Res = %d,\n", res);

	if(res < 0)
		printk(KERN_INFO "Error while sending back to user\n");
}

int init_netlink(void)
{
	nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, 0, NULL,NULL,THIS_MODULE);
	if(!nl_sk)
	{
		printk(KERN_ALERT "Error creating socket.\n");
		return -10;
	}
	return 0;
}

void netlink_exit(void)
{
	printk(KERN_INFO "exiting hello module\n");
	netlink_kernel_release(nl_sk);
}
