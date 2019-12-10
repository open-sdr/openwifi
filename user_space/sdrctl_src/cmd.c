// Xianjun jiao. putaoshu@msn.com; xianjun.jiao@imec.be;

#include <stdbool.h>
#include <errno.h>
#include <net/if.h>
#include <strings.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include "nl80211.h"
#include "sdrctl.h"
#include "nl80211_testmode_def.h"


static int cb_reg_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	struct nlattr *tb[OPENWIFI_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	//printf("cb_reg_handler\n");

	nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!attrs[NL80211_ATTR_TESTDATA])
		return NL_SKIP;

	nla_parse(tb, OPENWIFI_ATTR_MAX, nla_data(attrs[NL80211_ATTR_TESTDATA]), nla_len(attrs[NL80211_ATTR_TESTDATA]), NULL);

	//printf("reg addr: %08x\n", nla_get_u32(tb[REG_ATTR_ADDR]));
	printf("reg  val: %08x\n", nla_get_u32(tb[REG_ATTR_VAL]));

	return NL_SKIP;
}

static int handle_set_reg(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;
	char *end;
	unsigned int reg_cat, reg_addr, reg_val;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata) {
		return 1;
	}

	if (strcasecmp(argv[0],"rf")==0)
		reg_cat=1;
	else if (strcasecmp(argv[0],"rx_intf")==0)
		reg_cat = 2;
	else if (strcasecmp(argv[0],"tx_intf")==0)
		reg_cat = 3;
	else if (strcasecmp(argv[0],"rx")==0)
		reg_cat = 4;
	else if (strcasecmp(argv[0],"tx")==0)
		reg_cat = 5;
	else if (strcasecmp(argv[0],"xpu")==0)
		reg_cat = 6;
	else if (strcasecmp(argv[0],"drv_rx")==0)
		reg_cat = 7;
	else if (strcasecmp(argv[0],"drv_tx")==0)
		reg_cat = 8;
	else if (strcasecmp(argv[0],"drv_xpu")==0)
		reg_cat = 9;
	else {
		printf("Wrong the 1st argument. Should be rf/rx_intf/tx_intf/rx/tx/xpu/drv_rx/drv_tx/drv_xpu\n");
		return 1;
	}

	reg_addr = strtoul(argv[1], &end, 10);
	if (*end) {
		return 1;
	}
	reg_addr = reg_addr<<2;//from idx to addr
	reg_addr = ((reg_cat<<16)|reg_addr);

	reg_val = strtoul(argv[2], &end, 10);
	if (*end) {
		return 1;
	}

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, REG_CMD_SET);
	NLA_PUT_U32(msg, REG_ATTR_ADDR, reg_addr);
	NLA_PUT_U32(msg, REG_ATTR_VAL,  reg_val);

	nla_nest_end(msg, tmdata);

	printf("reg  cat: %d\n",   reg_cat);
	printf("reg addr: %08x\n", reg_addr);
	printf("reg  val: %08x\n", reg_val);

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, reg, "<rf/rx_intf/tx_intf/rx/tx/xpu/drv_rx/drv_tx/drv_xpu reg_idx value>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_set_reg, "set reg");

static int handle_get_reg(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	char *end;
	struct nlattr *tmdata;
	unsigned int reg_cat, reg_addr;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata)
		return 1;

	if (strcasecmp(argv[0],"rf")==0)
		reg_cat=1;
	else if (strcasecmp(argv[0],"rx_intf")==0)
		reg_cat = 2;
	else if (strcasecmp(argv[0],"tx_intf")==0)
		reg_cat = 3;
	else if (strcasecmp(argv[0],"rx")==0)
		reg_cat = 4;
	else if (strcasecmp(argv[0],"tx")==0)
		reg_cat = 5;
	else if (strcasecmp(argv[0],"xpu")==0)
		reg_cat = 6;
	else if (strcasecmp(argv[0],"drv_rx")==0)
		reg_cat = 7;
	else if (strcasecmp(argv[0],"drv_tx")==0)
		reg_cat = 8;
	else if (strcasecmp(argv[0],"drv_xpu")==0)
		reg_cat = 9;
	else {
		printf("Wrong the 1st argument. Should be rf/rx_intf/tx_intf/rx/tx/xpu/drv_rx/drv_tx/drv_xpu\n");
		return 1;
	}

	reg_addr = strtoul(argv[1], &end, 10);
	if (*end) {
		return 1;
	}
	reg_addr = reg_addr<<2;//from idx to addr
	reg_addr = ((reg_cat<<16)|reg_addr);
	printf("SENDaddr: %08x\n", reg_addr);

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, REG_CMD_GET);
	NLA_PUT_U32(msg, REG_ATTR_ADDR, reg_addr);

	nla_nest_end(msg, tmdata);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, cb_reg_handler, NULL);
	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(get, reg, "<rf/rx_intf/tx_intf/rx/tx/xpu/drv_rx/drv_tx/drv_xpu reg_idx>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_get_reg, "get reg");

static int cb_openwifi_rssi_th_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	struct nlattr *tb[OPENWIFI_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!attrs[NL80211_ATTR_TESTDATA])
		return NL_SKIP;

	nla_parse(tb, OPENWIFI_ATTR_MAX, nla_data(attrs[NL80211_ATTR_TESTDATA]), nla_len(attrs[NL80211_ATTR_TESTDATA]), NULL);

	printf("openwifi rssi_th: %d\n", nla_get_u32(tb[OPENWIFI_ATTR_RSSI_TH]));

	return NL_SKIP;
}

static int handle_set_rssi_th(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;
	char *end;
	unsigned int tmp;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata) {
		return 1;
	}

	tmp = strtoul(argv[0], &end, 10);

	if (*end) {
		return 1;
	}

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_SET_RSSI_TH);
	NLA_PUT_U32(msg, OPENWIFI_ATTR_RSSI_TH, tmp);

	nla_nest_end(msg, tmdata);

	printf("openwifi rssi_th: %d\n", tmp);

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, rssi_th, "<rssi_th in value>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_set_rssi_th, "set rssi_th");

static int handle_get_rssi_th(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata)
		return 1;

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_GET_RSSI_TH);

	nla_nest_end(msg, tmdata);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, cb_openwifi_rssi_th_handler, NULL);
	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(get, rssi_th, "<rssi_th in value>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_get_rssi_th, "get rssi_th");

static int cb_openwifi_slice_total0_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	struct nlattr *tb[OPENWIFI_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!attrs[NL80211_ATTR_TESTDATA])
		return NL_SKIP;

	nla_parse(tb, OPENWIFI_ATTR_MAX, nla_data(attrs[NL80211_ATTR_TESTDATA]), nla_len(attrs[NL80211_ATTR_TESTDATA]), NULL);

	printf("openwifi slice_total0 (duration): %dus\n", nla_get_u32(tb[OPENWIFI_ATTR_SLICE_TOTAL0]));

	return NL_SKIP;
}

static int handle_set_slice_total0(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;
	char *end;
	unsigned int tmp;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata) {
		return 1;
	}

	tmp = strtoul(argv[0], &end, 10);

	if (*end) {
		return 1;
	}

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_SET_SLICE_TOTAL0);
	NLA_PUT_U32(msg, OPENWIFI_ATTR_SLICE_TOTAL0, tmp);

	nla_nest_end(msg, tmdata);

	printf("openwifi slice_total0 (duration): %dus\n", tmp);

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, slice_total0, "<slice_total0(duration) in us>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_set_slice_total0, "set slice_total0");

static int handle_get_slice_total0(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata)
		return 1;

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_GET_SLICE_TOTAL0);

	nla_nest_end(msg, tmdata);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, cb_openwifi_slice_total0_handler, NULL);
	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(get, slice_total0, "<slice_total0(duration) in us>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_get_slice_total0, "get slice_total0");


static int cb_openwifi_slice_total1_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	struct nlattr *tb[OPENWIFI_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!attrs[NL80211_ATTR_TESTDATA])
		return NL_SKIP;

	nla_parse(tb, OPENWIFI_ATTR_MAX, nla_data(attrs[NL80211_ATTR_TESTDATA]), nla_len(attrs[NL80211_ATTR_TESTDATA]), NULL);

	printf("openwifi slice_total1 (duration): %dus\n", nla_get_u32(tb[OPENWIFI_ATTR_SLICE_TOTAL1]));

	return NL_SKIP;
}

static int handle_set_slice_total1(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;
	char *end;
	unsigned int tmp;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata) {
		return 1;
	}

	tmp = strtoul(argv[0], &end, 10);

	if (*end) {
		return 1;
	}

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_SET_SLICE_TOTAL1);
	NLA_PUT_U32(msg, OPENWIFI_ATTR_SLICE_TOTAL1, tmp);

	nla_nest_end(msg, tmdata);

	printf("openwifi slice_total1 (duration): %dus\n", tmp);

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, slice_total1, "<slice_total1(duration) in us>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_set_slice_total1, "set slice_total1");

static int handle_get_slice_total1(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata)
		return 1;

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_GET_SLICE_TOTAL1);

	nla_nest_end(msg, tmdata);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, cb_openwifi_slice_total1_handler, NULL);
	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(get, slice_total1, "<slice_total1(duration) in us>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_get_slice_total1, "get slice_total1");


static int cb_openwifi_slice_start0_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	struct nlattr *tb[OPENWIFI_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!attrs[NL80211_ATTR_TESTDATA])
		return NL_SKIP;

	nla_parse(tb, OPENWIFI_ATTR_MAX, nla_data(attrs[NL80211_ATTR_TESTDATA]), nla_len(attrs[NL80211_ATTR_TESTDATA]), NULL);

	printf("openwifi slice_start0 (duration): %dus\n", nla_get_u32(tb[OPENWIFI_ATTR_SLICE_START0]));

	return NL_SKIP;
}

static int handle_set_slice_start0(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;
	char *end;
	unsigned int tmp;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata) {
		return 1;
	}

	tmp = strtoul(argv[0], &end, 10);

	if (*end) {
		return 1;
	}

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_SET_SLICE_START0);
	NLA_PUT_U32(msg, OPENWIFI_ATTR_SLICE_START0, tmp);

	nla_nest_end(msg, tmdata);

	printf("openwifi slice_start0 (duration): %dus\n", tmp);

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, slice_start0, "<slice_start0(duration) in us>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_set_slice_start0, "set slice_start0");

static int handle_get_slice_start0(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata)
		return 1;

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_GET_SLICE_START0);

	nla_nest_end(msg, tmdata);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, cb_openwifi_slice_start0_handler, NULL);
	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(get, slice_start0, "<slice_start0(duration) in us>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_get_slice_start0, "get slice_start0");


static int cb_openwifi_slice_start1_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	struct nlattr *tb[OPENWIFI_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!attrs[NL80211_ATTR_TESTDATA])
		return NL_SKIP;

	nla_parse(tb, OPENWIFI_ATTR_MAX, nla_data(attrs[NL80211_ATTR_TESTDATA]), nla_len(attrs[NL80211_ATTR_TESTDATA]), NULL);

	printf("openwifi slice_start1 (duration): %dus\n", nla_get_u32(tb[OPENWIFI_ATTR_SLICE_START1]));

	return NL_SKIP;
}

static int handle_set_slice_start1(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;
	char *end;
	unsigned int tmp;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata) {
		return 1;
	}

	tmp = strtoul(argv[0], &end, 10);

	if (*end) {
		return 1;
	}

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_SET_SLICE_START1);
	NLA_PUT_U32(msg, OPENWIFI_ATTR_SLICE_START1, tmp);

	nla_nest_end(msg, tmdata);

	printf("openwifi slice_start1 (duration): %dus\n", tmp);

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, slice_start1, "<slice_start1(duration) in us>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_set_slice_start1, "set slice_start1");

static int handle_get_slice_start1(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata)
		return 1;

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_GET_SLICE_START1);

	nla_nest_end(msg, tmdata);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, cb_openwifi_slice_start1_handler, NULL);
	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(get, slice_start1, "<slice_start1(duration) in us>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_get_slice_start1, "get slice_start1");


static int cb_openwifi_slice_end0_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	struct nlattr *tb[OPENWIFI_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!attrs[NL80211_ATTR_TESTDATA])
		return NL_SKIP;

	nla_parse(tb, OPENWIFI_ATTR_MAX, nla_data(attrs[NL80211_ATTR_TESTDATA]), nla_len(attrs[NL80211_ATTR_TESTDATA]), NULL);

	printf("openwifi slice_end0 (duration): %dus\n", nla_get_u32(tb[OPENWIFI_ATTR_SLICE_END0]));

	return NL_SKIP;
}

static int handle_set_slice_end0(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;
	char *end;
	unsigned int tmp;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata) {
		return 1;
	}

	tmp = strtoul(argv[0], &end, 10);

	if (*end) {
		return 1;
	}

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_SET_SLICE_END0);
	NLA_PUT_U32(msg, OPENWIFI_ATTR_SLICE_END0, tmp);

	nla_nest_end(msg, tmdata);

	printf("openwifi slice_end0 (duration): %dus\n", tmp);

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, slice_end0, "<slice_end0(duration) in us>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_set_slice_end0, "set slice_end0");

static int handle_get_slice_end0(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata)
		return 1;

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_GET_SLICE_END0);

	nla_nest_end(msg, tmdata);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, cb_openwifi_slice_end0_handler, NULL);
	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(get, slice_end0, "<slice_end0(duration) in us>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_get_slice_end0, "get slice_end0");


static int cb_openwifi_slice_end1_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	struct nlattr *tb[OPENWIFI_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!attrs[NL80211_ATTR_TESTDATA])
		return NL_SKIP;

	nla_parse(tb, OPENWIFI_ATTR_MAX, nla_data(attrs[NL80211_ATTR_TESTDATA]), nla_len(attrs[NL80211_ATTR_TESTDATA]), NULL);

	printf("openwifi slice_end1 (duration): %dus\n", nla_get_u32(tb[OPENWIFI_ATTR_SLICE_END1]));

	return NL_SKIP;
}

static int handle_set_slice_end1(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;
	char *end;
	unsigned int tmp;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata) {
		return 1;
	}

	tmp = strtoul(argv[0], &end, 10);

	if (*end) {
		return 1;
	}

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_SET_SLICE_END1);
	NLA_PUT_U32(msg, OPENWIFI_ATTR_SLICE_END1, tmp);

	nla_nest_end(msg, tmdata);

	printf("openwifi slice_end1 (duration): %dus\n", tmp);

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, slice_end1, "<slice_end1(duration) in us>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_set_slice_end1, "set slice_end1");

static int handle_get_slice_end1(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata)
		return 1;

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_GET_SLICE_END1);

	nla_nest_end(msg, tmdata);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, cb_openwifi_slice_end1_handler, NULL);
	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(get, slice_end1, "<slice_end1(duration) in us>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_get_slice_end1, "get slice_end1");


static int cb_openwifi_slice0_target_mac_addr_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	struct nlattr *tb[OPENWIFI_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!attrs[NL80211_ATTR_TESTDATA])
		return NL_SKIP;

	nla_parse(tb, OPENWIFI_ATTR_MAX, nla_data(attrs[NL80211_ATTR_TESTDATA]), nla_len(attrs[NL80211_ATTR_TESTDATA]), NULL);

	printf("openwifi slice0_target_mac_addr(low32) in hex: %08x\n", nla_get_u32(tb[OPENWIFI_ATTR_ADDR0]));

	return NL_SKIP;
}

static int handle_set_addr0(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;
	char *end;
	unsigned int slice0_target_mac_addr;

	//printf("handle_set_slice0_target_mac_addr\n");
	
	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata) {
		//printf("handle_set_slice0_target_mac_addr 1\n");
		return 1;
	}

	slice0_target_mac_addr = strtoul(argv[0], &end, 16);

	if (*end) {
		//printf("handle_set_slice0_target_mac_addr 2 %d\n", slice0_target_mac_addr);
		return 1;
	}

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_SET_ADDR0);
	NLA_PUT_U32(msg, OPENWIFI_ATTR_ADDR0, slice0_target_mac_addr);

	nla_nest_end(msg, tmdata);

	printf("openwifi slice0_target_mac_addr(low32) in hex: %08x\n", slice0_target_mac_addr);

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, addr0, "<slice0_target_mac_addr(low32) in hex>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_set_addr0, "set addr0");

static int handle_get_slice0_target_mac_addr(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata)
		return 1;

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_GET_ADDR0);

	nla_nest_end(msg, tmdata);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, cb_openwifi_slice0_target_mac_addr_handler, NULL);
	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(get, addr0, "<slice0_target_mac_addr(low32) in hex>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_get_slice0_target_mac_addr, "get addr0");

static int cb_openwifi_slice1_target_mac_addr_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	struct nlattr *tb[OPENWIFI_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!attrs[NL80211_ATTR_TESTDATA])
		return NL_SKIP;

	nla_parse(tb, OPENWIFI_ATTR_MAX, nla_data(attrs[NL80211_ATTR_TESTDATA]), nla_len(attrs[NL80211_ATTR_TESTDATA]), NULL);

	printf("openwifi slice1_target_mac_addr(low32) in hex: %08x\n", nla_get_u32(tb[OPENWIFI_ATTR_ADDR1]));

	return NL_SKIP;
}

static int handle_set_slice1_target_mac_addr(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;
	char *end;
	unsigned int slice1_target_mac_addr;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata)
		return 1;

	slice1_target_mac_addr = strtoul(argv[0], &end, 16);

	if (*end)
		return 1;

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_SET_ADDR1);
	NLA_PUT_U32(msg, OPENWIFI_ATTR_ADDR1, slice1_target_mac_addr);

	nla_nest_end(msg, tmdata);

	printf("openwifi slice1_target_mac_addr(low32) in hex: %08x\n", slice1_target_mac_addr);

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, addr1, "<slice1_target_mac_addr(low32) in hex>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_set_slice1_target_mac_addr, "set addr1");

static int handle_get_slice1_target_mac_addr(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata)
		return 1;

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_GET_ADDR1);

	nla_nest_end(msg, tmdata);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, cb_openwifi_slice1_target_mac_addr_handler, NULL);
	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(get, addr1, "<slice1_target_mac_addr(low32) in hex>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_get_slice1_target_mac_addr, "get addr1");

static int cb_openwifi_gap_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	struct nlattr *tb[OPENWIFI_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!attrs[NL80211_ATTR_TESTDATA])
		return NL_SKIP;

	nla_parse(tb, OPENWIFI_ATTR_MAX, nla_data(attrs[NL80211_ATTR_TESTDATA]), nla_len(attrs[NL80211_ATTR_TESTDATA]), NULL);

	printf("openwifi GAP (usec): %d\n", nla_get_u32(tb[OPENWIFI_ATTR_GAP]));

	return NL_SKIP;
}

static int handle_set_gap(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;
	char *end;
	unsigned int gap_us;

	//printf("handle_set_gap\n");
	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata)
		return 1;

	gap_us = strtoul(argv[0], &end, 10);

	if (*end)
		return 1;

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_SET_GAP);
	NLA_PUT_U32(msg, OPENWIFI_ATTR_GAP, gap_us);

	nla_nest_end(msg, tmdata);
	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, gap, "<gap in usec>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_set_gap, "set inter frame gap of openwifi radio");

static int handle_get_gap(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	struct nlattr *tmdata;

	tmdata = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!tmdata)
		return 1;

	NLA_PUT_U32(msg, OPENWIFI_ATTR_CMD, OPENWIFI_CMD_GET_GAP);

	nla_nest_end(msg, tmdata);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, cb_openwifi_gap_handler, NULL);
	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(get, gap, "<gap in usec>", NL80211_CMD_TESTMODE, 0, CIB_NETDEV, handle_get_gap, "get inter frame gap of openwifi radio");
