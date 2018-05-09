/*
 * net/tipc/bearer.h: Include file for TIPC bearer code
 *
 * Copyright (c) 1996-2006, Ericsson AB
 * Copyright (c) 2005, 2010-2011, Wind River Systems
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _TIPC_BEARER_H
#define _TIPC_BEARER_H

#include "bcast.h"

//node---bearer1---media1
//     |    +---   ---+
//     |         \/
//	   |		 /\
//     |    +---   ---+ 
//     |-bearer2---media2

#define MAX_BEARERS	2	//目前一个节点最多支持2个网卡同时服务
#define MAX_MEDIA	2	//目前一个节点最多支持2个media同时服务(貌似一般只用ETH，IB几乎不用，所以这里一般认为media就是1个)

/*
 * Identifiers associated with TIPC message header media address info
 *
 * - address info field is 20 bytes long
 * - media type identifier located at offset 3
 * - remaining bytes vary according to media type
 */
#define TIPC_MEDIA_ADDR_SIZE	20
#define TIPC_MEDIA_TYPE_OFFSET	3

/*
 * Identifiers of supported TIPC media types
 */
#define TIPC_MEDIA_TYPE_ETH	1
#define TIPC_MEDIA_TYPE_IB	2

/**
 * struct tipc_media_addr - destination address used by TIPC bearers
 * @value: address info (format defined by media)
 * @media_id: TIPC media type identifier
 * @broadcast: non-zero if address is a broadcast address
 */
struct tipc_media_addr {
	u8 value[TIPC_MEDIA_ADDR_SIZE];	//MAC地址(media是Ethernet时)，TIPC_MEDIA_ADDR_SIZE=20，MAC地址只要6个字节，所以后12个字节全置0，20是为了兼容IB
	u8 media_id;					//第几个media，最多2个，所以这个值要么是0，要么是1
	u8 broadcast;					//0-非广播MAC，非0-广播MAC(media是Ethernet时)
};

struct tipc_bearer;

/**
 * struct tipc_media - TIPC media information available to internal users
 * @send_msg: routine which handles buffer transmission
 * @enable_bearer: routine which enables a bearer
 * @disable_bearer: routine which disables a bearer
 * @addr2str: routine which converts media address to string
 * @addr2msg: routine which converts media address to protocol message area
 * @msg2addr: routine which converts media address from protocol message area
 * @bcast_addr: media address used in broadcasting
 * @priority: default link (and bearer) priority
 * @tolerance: default time (in ms) before declaring link failure
 * @window: default window (in packets) before declaring link congestion
 * @type_id: TIPC media identifier
 * @name: media name
 */
/*[yangkun]  PS：ETH和IB属于链路层*/
struct tipc_media {
	int (*send_msg)(struct sk_buff *buf,
			struct tipc_bearer *b_ptr,
			struct tipc_media_addr *dest);
	int (*enable_bearer)(struct tipc_bearer *b_ptr);
	void (*disable_bearer)(struct tipc_bearer *b_ptr);
	int (*addr2str)(struct tipc_media_addr *a, char *str_buf, int str_size);
	int (*addr2msg)(struct tipc_media_addr *a, char *msg_area);
	int (*msg2addr)(const struct tipc_bearer *b_ptr,
			struct tipc_media_addr *a, char *msg_area);
	u32 priority;	//link的主备模式下，网卡服务优先级
	u32 tolerance;	//多久没收到对端报文(任何形式)，就声明"断链"，可以设置
	u32 window;		//网卡中排队达到多少个包，就声明"拥塞"，可以设置
	u32 type_id;	//ETH OR IB
	char name[TIPC_MAX_MEDIA_NAME];
};

/**
 * struct tipc_bearer - TIPC bearer structure
 * @usr_handle: pointer to additional media-specific information about bearer(initalized by media)
 * @mtu: max packet size bearer can support(initalized by media)
 * @blocked: non-zero if bearer is blocked(initalized by media)
 * @lock: spinlock for controlling access to bearer(initalized by media)
 * @addr: media-specific address associated with bearer
 * @name: bearer name (format = media:interface)
 * @media: ptr to media structure associated with bearer
 * @priority: default link priority for bearer
 * @window: default window size for bearer
 * @tolerance: default link tolerance for bearer
 * @identity: array index of this bearer within TIPC bearer array
 * @link_req: ptr to (optional) structure making periodic link setup requests
 * @links: list of non-congested links associated with bearer
 * @active: non-zero if bearer structure is represents a bearer
 * @net_plane: network plane ('A' through 'H') currently associated with bearer
 * @nodes: indicates which nodes in cluster can be reached through bearer
 *
 * Note: media-specific code is responsible for initialization of the fields
 * indicated below when a bearer is enabled; TIPC's generic bearer code takes
 * care of initializing all other fields.
 */
struct tipc_bearer {
	void *usr_handle;					//指向任意类型的指针，留给用户自由发挥
	u32 mtu;							//网卡能支撑的最大包尺寸，这里为66000，一个包最多66000个字节
	int blocked;						//1-网卡blocked了，0-网卡正常
	struct tipc_media_addr addr;		
	char name[TIPC_MAX_BEARER_NAME];	//格式为eth:eth0
	spinlock_t lock;
	struct tipc_media *media;			//网卡依赖的链路层数据结构(ETH OR IB)
	struct tipc_media_addr bcast_addr;
	u32 priority;						//“默认”优先级，用途和tipc_media中的一样，此处是默认值
	u32 window;							//“默认”窗口值，用途和tipc_media中的一样，此处是默认值
	u32 tolerance;						//“默认”容忍值，用途和tipc_media中的一样，此处是默认值
	u32 identity;						//本节点中可能有多个bearer，每个都有一个实例，这些实例以数组形式存在tipc_bearer[]数组中，此值为数组下标
	struct tipc_link_req *link_req;		//建链事务实例
	struct list_head links;				//已持(非阻塞)链路列表
	int active;							//1-使能的，0-失能的
	char net_plane;						//网络平面(A~H)？？？常说的"切平面"估计就是指这个
	struct tipc_node_map nodes;			//节点地图
};

/*[yangkun]  media_name表示链路层使用哪种协议，if_name表示在media_name指示的链路层中被注册的名字*/
struct tipc_bearer_names {
	char media_name[TIPC_MAX_MEDIA_NAME];	//比如eth
	char if_name[TIPC_MAX_IF_NAME];			//比如eth0 
};

struct tipc_link;

extern struct tipc_bearer tipc_bearers[];

/*
 * TIPC routines available to supported media types
 */
int tipc_register_media(struct tipc_media *m_ptr);

void tipc_recv_msg(struct sk_buff *buf, struct tipc_bearer *tb_ptr);

int  tipc_block_bearer(const char *name);
void tipc_continue(struct tipc_bearer *tb_ptr);

int tipc_enable_bearer(const char *bearer_name, u32 disc_domain, u32 priority);
int tipc_disable_bearer(const char *name);

/*
 * Routines made available to TIPC by supported media types
 */
int  tipc_eth_media_start(void);
void tipc_eth_media_stop(void);

#ifdef CONFIG_TIPC_MEDIA_IB
int  tipc_ib_media_start(void);
void tipc_ib_media_stop(void);
#else
static inline int tipc_ib_media_start(void) { return 0; }
static inline void tipc_ib_media_stop(void) { return; }
#endif

int tipc_media_set_priority(const char *name, u32 new_value);
int tipc_media_set_window(const char *name, u32 new_value);
void tipc_media_addr_printf(char *buf, int len, struct tipc_media_addr *a);
struct sk_buff *tipc_media_get_names(void);

struct sk_buff *tipc_bearer_get_names(void);
void tipc_bearer_add_dest(struct tipc_bearer *b_ptr, u32 dest);
void tipc_bearer_remove_dest(struct tipc_bearer *b_ptr, u32 dest);
struct tipc_bearer *tipc_bearer_find(const char *name);
struct tipc_bearer *tipc_bearer_find_interface(const char *if_name);
struct tipc_media *tipc_media_find(const char *name);
int tipc_bearer_blocked(struct tipc_bearer *b_ptr);
void tipc_bearer_stop(void);

/**
 * tipc_bearer_send- sends buffer to destination over bearer
 *
 * IMPORTANT:
 * The media send routine must not alter the buffer being passed in
 * as it may be needed for later retransmission!
 */
static inline void tipc_bearer_send(struct tipc_bearer *b, struct sk_buff *buf,
				   struct tipc_media_addr *dest)
{
	b->media->send_msg(buf, b, dest);
}

#endif	/* _TIPC_BEARER_H */
