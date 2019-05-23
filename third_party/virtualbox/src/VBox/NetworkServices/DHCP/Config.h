/* $Id: Config.h $ */
/** @file
 * Config.h
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <iprt/asm-math.h>
#include <iprt/cpp/utils.h>

#include <VBox/com/ptr.h>
#include <VBox/com/string.h>
#include <VBox/com/VirtualBox.h>

#include "../NetLib/cpp/utils.h"


class RawOption
{
public:
    RawOption()
    {
        /** @todo r=bird: this is crazy. */
        RT_ZERO(*this);
    }
    uint8_t u8OptId;
    uint8_t cbRawOpt;
    uint8_t au8RawOpt[255];
};

class ClientData;
class Client;
class Lease;
class BaseConfigEntity;

class NetworkConfigEntity;
class HostConfigEntity;
class ClientMatchCriteria;
class ConfigurationManager;

/*
 * it's a basic representation of
 * of out undestanding what client is
 * XXX: Client might sends Option 61 (RFC2132 9.14 "Client-identifier") signalling
 * that we may identify it in special way
 *
 * XXX: Client might send Option 60 (RFC2132 9.13 "Vendor class undentifier")
 * in response it's expected server sends Option 43 (RFC2132 8.4. "Vendor Specific Information")
 */
class Client
{
    friend class Lease;
    friend class ConfigurationManager;

    public:
    Client();
    void initWithMac(const RTMAC& mac);
    bool operator== (const RTMAC& mac) const;
    const RTMAC& getMacAddress() const;

    /** Dumps client query */
    void dump();

    Lease lease();
    const Lease lease() const;

    public:
    static const Client NullClient;

    private:
    Client(ClientData *);
    SharedPtr<ClientData> m;
};


bool operator== (const Lease&, const Lease&);
bool operator!= (const Lease&, const Lease&);
bool operator< (const Lease&, const Lease&);


typedef std::map<uint8_t, RawOption> MapOptionId2RawOption;
typedef MapOptionId2RawOption::iterator MapOptionId2RawOptionIterator;
typedef MapOptionId2RawOption::const_iterator MapOptionId2RawOptionConstIterator;
typedef MapOptionId2RawOption::value_type MapOptionId2RawOptionValue;

namespace xml {
    class ElementNode;
}

class Lease
{
    friend class Client;
    friend bool operator== (const Lease&, const Lease&);
    //friend int ConfigurationManager::loadFromFile(const std::string&);
    friend class ConfigurationManager;

    public:
    Lease();
    Lease(const Client&);

    bool isExpired() const;
    void expire();

    /* Depending on phase *Expiration and phaseStart initialize different values. */
    void bindingPhase(bool);
    void phaseStart(uint64_t u64Start);
    bool isInBindingPhase() const;
    /* returns 0 if in binding state */
    uint64_t issued() const;

    void setExpiration(uint32_t);
    uint32_t getExpiration() const;

    RTNETADDRIPV4 getAddress() const;
    void setAddress(RTNETADDRIPV4);

    const NetworkConfigEntity *getConfig() const;
    void setConfig(NetworkConfigEntity *);

    const MapOptionId2RawOption& options() const;

    bool toXML(xml::ElementNode *) const;
    bool fromXML(const xml::ElementNode *);

    public:
    static const Lease NullLease;

    private:
    Lease(ClientData *);
    SharedPtr<ClientData> m;
};


typedef std::vector<Client> VecClient;
typedef VecClient::iterator VecClientIterator;
typedef VecClient::const_iterator VecClientConstIterator;

typedef std::vector<RTMAC> MacAddressContainer;
typedef MacAddressContainer::iterator MacAddressIterator;

typedef std::vector<RTNETADDRIPV4> Ipv4AddressContainer;
typedef Ipv4AddressContainer::iterator Ipv4AddressIterator;
typedef Ipv4AddressContainer::const_iterator Ipv4AddressConstIterator;

typedef std::map<Lease, RTNETADDRIPV4> MapLease2Ip4Address;
typedef MapLease2Ip4Address::iterator MapLease2Ip4AddressIterator;
typedef MapLease2Ip4Address::const_iterator MapLease2Ip4AddressConstIterator;
typedef MapLease2Ip4Address::value_type MapLease2Ip4AddressPair;

/**
 *
 */
class ClientMatchCriteria
{
    public:
    virtual bool check(const Client&) const {return false;};
};


class ORClientMatchCriteria: ClientMatchCriteria
{
    ClientMatchCriteria* m_left;
    ClientMatchCriteria* m_right;
    ORClientMatchCriteria(ClientMatchCriteria *left, ClientMatchCriteria *right)
    {
        m_left = left;
        m_right = right;
    }

    virtual bool check(const Client& client) const
    {
        return (m_left->check(client) || m_right->check(client));
    }
};


class ANDClientMatchCriteria: ClientMatchCriteria
{
public:
    ANDClientMatchCriteria(ClientMatchCriteria *left, ClientMatchCriteria *right)
    {
        m_left = left;
        m_right = right;
    }

    virtual bool check(const Client& client) const
    {
        return (m_left->check(client) && m_right->check(client));
    }

private:
    ClientMatchCriteria* m_left;
    ClientMatchCriteria* m_right;

};


class AnyClientMatchCriteria: public ClientMatchCriteria
{
public:
    virtual bool check(const Client&) const
    {
        return true;
    }
};


class MACClientMatchCriteria: public ClientMatchCriteria
{
public:
    MACClientMatchCriteria(const RTMAC& mac):m_mac(mac){}

    virtual bool check(const Client& client) const;

private:
    RTMAC m_mac;
};


#if 0
/* XXX: Later */
class VmSlotClientMatchCriteria: public ClientMatchCriteria
{
    str::string VmName;
    uint8_t u8Slot;
    virtual bool check(const Client& client)
    {
        return (   client.VmName == VmName
                   && (   u8Slot == (uint8_t)~0 /* any */
                       || client.u8Slot == u8Slot));
    }
};
#endif


/* Option 60 */
class ClassClientMatchCriteria: ClientMatchCriteria{};
/* Option 61 */
class ClientIdentifierMatchCriteria: ClientMatchCriteria{};


class BaseConfigEntity
{
    public:
    BaseConfigEntity(const ClientMatchCriteria *criteria = NULL,
      int matchingLevel = 0)
      : m_criteria(criteria),
      m_MatchLevel(matchingLevel){};
    virtual ~BaseConfigEntity(){};
    /* XXX */
    int add(BaseConfigEntity *cfg)
    {
        m_children.push_back(cfg);
        return 0;
    }

    /* Should return how strong matching */
    virtual int match(Client& client, BaseConfigEntity **cfg);
    virtual uint32_t expirationPeriod() const = 0;

    protected:
    const ClientMatchCriteria *m_criteria;
    int m_MatchLevel;
    std::vector<BaseConfigEntity *> m_children;
};


class NullConfigEntity: public BaseConfigEntity
{
    public:
    NullConfigEntity(){}
    virtual ~NullConfigEntity(){}
    int add(BaseConfigEntity *) const { return 0;}
    virtual uint32_t expirationPeriod() const {return 0;}
};


class ConfigEntity: public BaseConfigEntity
{
    public:
    /* range */
    /* match conditions */
    ConfigEntity(std::string& name,
                 const BaseConfigEntity *cfg,
                 const ClientMatchCriteria *criteria,
                 int matchingLevel = 0):
      BaseConfigEntity(criteria, matchingLevel),
      m_name(name),
      m_parentCfg(cfg),
      m_u32ExpirationPeriod(0)
    {
        unconst(m_parentCfg)->add(this);
    }

    virtual uint32_t expirationPeriod() const
    {
        if (!m_u32ExpirationPeriod)
            return m_parentCfg->expirationPeriod();
        else
            return m_u32ExpirationPeriod;
    }

    /* XXX: private:*/
    std::string m_name;
    const BaseConfigEntity *m_parentCfg;
    uint32_t m_u32ExpirationPeriod;
};


/**
 * Network specific entries
 */
class NetworkConfigEntity:public ConfigEntity
{
public:
    /* Address Pool matching with network declaration */
    NetworkConfigEntity(std::string name,
                        const BaseConfigEntity *cfg,
                        const ClientMatchCriteria *criteria,
                        int matchlvl,
                        const RTNETADDRIPV4& networkID,
                        const RTNETADDRIPV4& networkMask,
                        const RTNETADDRIPV4& lowerIP,
                        const RTNETADDRIPV4& upperIP):
      ConfigEntity(name, cfg, criteria, matchlvl),
      m_NetworkID(networkID),
      m_NetworkMask(networkMask),
      m_UpperIP(upperIP),
      m_LowerIP(lowerIP)
    {
    };

    NetworkConfigEntity(std::string name,
                        const BaseConfigEntity *cfg,
                        const ClientMatchCriteria *criteria,
                        const RTNETADDRIPV4& networkID,
                        const RTNETADDRIPV4& networkMask):
      ConfigEntity(name, cfg, criteria, 5),
      m_NetworkID(networkID),
      m_NetworkMask(networkMask)
    {
        m_UpperIP.u = m_NetworkID.u | (~m_NetworkMask.u);
        m_LowerIP.u = m_NetworkID.u;
    };

    const RTNETADDRIPV4& upperIp() const {return m_UpperIP;}
    const RTNETADDRIPV4& lowerIp() const {return m_LowerIP;}
    const RTNETADDRIPV4& networkId() const {return m_NetworkID;}
    const RTNETADDRIPV4& netmask() const {return m_NetworkMask;}

    private:
    RTNETADDRIPV4 m_NetworkID;
    RTNETADDRIPV4 m_NetworkMask;
    RTNETADDRIPV4 m_UpperIP;
    RTNETADDRIPV4 m_LowerIP;
};


/**
 * Host specific entry
 * Address pool is contains one element
 */
class HostConfigEntity: public NetworkConfigEntity
{
public:
    HostConfigEntity(const RTNETADDRIPV4& addr,
                     std::string name,
                     const NetworkConfigEntity *cfg,
                     const ClientMatchCriteria *criteria):
      NetworkConfigEntity(name,
                          static_cast<const ConfigEntity*>(cfg), criteria, 10,
                          cfg->networkId(), cfg->netmask(), addr, addr)
    {
        /* upper addr == lower addr */
    }
};

class RootConfigEntity: public NetworkConfigEntity
{
public:
    RootConfigEntity(std::string name, uint32_t expirationPeriod);
    virtual ~RootConfigEntity(){};
};


#if 0
/**
 * Shared regions e.g. some of configured networks declarations
 * are cover each other.
 * XXX: Shared Network is join on Network config entities with possible
 * overlaps in address pools. for a moment we won't configure and use them them
 */
class SharedNetworkConfigEntity: public NetworkEntity
{
public:
    SharedNetworkConfigEntity(){}
    int match(const Client& client) const { return m_criteria.match(client)? 3 : 0;}

    SharedNetworkConfigEntity(NetworkEntity& network)
    {
        Networks.push_back(network);
    }
    virtual ~SharedNetworkConfigEntity(){}

    std::vector<NetworkConfigEntity> Networks;
};
#endif

class ConfigurationManager
{
public:
    static ConfigurationManager* getConfigurationManager();
    static int extractRequestList(PCRTNETBOOTP pDhcpMsg, size_t cbDhcpMsg, RawOption& rawOpt);

    int loadFromFile(const com::Utf8Str&);
    int saveToFile();
    /**
     *
     */
    Client getClientByDhcpPacket(const RTNETBOOTP *pDhcpMsg, size_t cbDhcpMsg);

    /**
     * XXX: it's could be done on DHCPOFFER or on DHCPACK (rfc2131 gives freedom here
     * 3.1.2, what is strict that allocation should do address check before real
     * allocation)...
     */
    Lease allocateLease4Client(const Client& client, PCRTNETBOOTP pDhcpMsg, size_t cbDhcpMsg);

    /**
     * We call this before DHCPACK sent and after DHCPREQUEST received ...
     * when requested configuration is acceptable.
     */
    int commitLease4Client(Client& client);

    /**
     * Expires client lease.
     */
    int expireLease4Client(Client& client);

    static int findOption(uint8_t uOption, PCRTNETBOOTP pDhcpMsg, size_t cbDhcpMsg, RawOption& opt);

    NetworkConfigEntity *addNetwork(NetworkConfigEntity *pCfg,
                                    const RTNETADDRIPV4& networkId,
                                    const RTNETADDRIPV4& netmask,
                                    RTNETADDRIPV4& UpperAddress,
                                    RTNETADDRIPV4& LowerAddress);

    HostConfigEntity *addHost(NetworkConfigEntity*, const RTNETADDRIPV4&, ClientMatchCriteria*);
    int addToAddressList(uint8_t u8OptId, RTNETADDRIPV4& address);
    int flushAddressList(uint8_t u8OptId);
    int setString(uint8_t u8OptId, const std::string& str);
    const std::string& getString(uint8_t u8OptId);
    const Ipv4AddressContainer& getAddressList(uint8_t u8OptId);

private:
    ConfigurationManager():m(NULL){}
    void init();

    ~ConfigurationManager();
    bool isAddressTaken(const RTNETADDRIPV4& addr, Lease& lease);
    bool isAddressTaken(const RTNETADDRIPV4& addr);

public:
    /* nulls */
    const Ipv4AddressContainer m_empty;
    const std::string    m_noString;

private:
    struct Data;
    Data *m;
};


class NetworkManager
{
public:
    static NetworkManager *getNetworkManager(ComPtr<IDHCPServer> aDhcpServer = ComPtr<IDHCPServer>());

    const RTNETADDRIPV4& getOurAddress() const;
    const RTNETADDRIPV4& getOurNetmask() const;
    const RTMAC& getOurMac() const;

    void setOurAddress(const RTNETADDRIPV4& aAddress);
    void setOurNetmask(const RTNETADDRIPV4& aNetmask);
    void setOurMac(const RTMAC& aMac);

    bool handleDhcpReqDiscover(PCRTNETBOOTP pDhcpMsg, size_t cb);
    bool handleDhcpReqRequest(PCRTNETBOOTP pDhcpMsg, size_t cb);
    bool handleDhcpReqDecline(PCRTNETBOOTP pDhcpMsg, size_t cb);
    bool handleDhcpReqRelease(PCRTNETBOOTP pDhcpMsg, size_t cb);

    void setService(const VBoxNetHlpUDPService *);
private:
    NetworkManager();
    ~NetworkManager();

    int offer4Client(const Client& lease, uint32_t u32Xid, uint8_t *pu8ReqList, int cReqList);
    int ack(const Client& lease, uint32_t u32Xid, uint8_t *pu8ReqList, int cReqList);
    int nak(const Client& lease, uint32_t u32Xid);

    int prepareReplyPacket4Client(const Client& client, uint32_t u32Xid);
    int doReply(const Client& client, const std::vector<RawOption>& extra);
    int processParameterReqList(const Client& client, const uint8_t *pu8ReqList, int cReqList, std::vector<RawOption>& extra);

private:
    static NetworkManager *g_NetworkManager;

private:
    struct Data;
    Data *m;

};


extern const ClientMatchCriteria *g_AnyClient;
extern RootConfigEntity *g_RootConfig;
extern const NullConfigEntity *g_NullConfig;

/**
 * Helper class for stuffing DHCP options into a reply packet.
 */
class VBoxNetDhcpWriteCursor
{
private:
    uint8_t        *m_pbCur;       /**< The current cursor position. */
    uint8_t        *m_pbEnd;       /**< The end the current option space. */
    uint8_t        *m_pfOverload;  /**< Pointer to the flags of the overload option. */
    uint8_t         m_fUsed;       /**< Overload fields that have been used. */
    PRTNETDHCPOPT   m_pOpt;        /**< The current option. */
    PRTNETBOOTP     m_pDhcp;       /**< The DHCP packet. */
    bool            m_fOverflowed; /**< Set if we've overflowed, otherwise false. */

public:
    /** Instantiate an option cursor for the specified DHCP message. */
    VBoxNetDhcpWriteCursor(PRTNETBOOTP pDhcp, size_t cbDhcp) :
        m_pbCur(&pDhcp->bp_vend.Dhcp.dhcp_opts[0]),
        m_pbEnd((uint8_t *)pDhcp + cbDhcp),
        m_pfOverload(NULL),
        m_fUsed(0),
        m_pOpt(NULL),
        m_pDhcp(pDhcp),
        m_fOverflowed(false)
    {
        AssertPtr(pDhcp);
        Assert(cbDhcp > RT_UOFFSETOF(RTNETBOOTP, bp_vend.Dhcp.dhcp_opts[10]));
    }

    /** Destructor.  */
    ~VBoxNetDhcpWriteCursor()
    {
        m_pbCur = m_pbEnd = m_pfOverload = NULL;
        m_pOpt = NULL;
        m_pDhcp = NULL;
    }

    /**
     * Try use the bp_file field.
     * @returns true if not overloaded, false otherwise.
     */
    bool useBpFile(void)
    {
        if (    m_pfOverload
            &&  (*m_pfOverload & 1))
            return false;
        m_fUsed |= 1 /* bp_file flag*/;
        return true;
    }


    /**
     * Try overload more BOOTP fields
     */
    bool overloadMore(void)
    {
        /* switch option area. */
        uint8_t    *pbNew;
        uint8_t    *pbNewEnd;
        uint8_t     fField;
        if (!(m_fUsed & 1))
        {
            fField     = 1;
            pbNew      = &m_pDhcp->bp_file[0];
            pbNewEnd   = &m_pDhcp->bp_file[sizeof(m_pDhcp->bp_file)];
        }
        else if (!(m_fUsed & 2))
        {
            fField     = 2;
            pbNew      = &m_pDhcp->bp_sname[0];
            pbNewEnd   = &m_pDhcp->bp_sname[sizeof(m_pDhcp->bp_sname)];
        }
        else
            return false;

        if (!m_pfOverload)
        {
            /* Add an overload option. */
            *m_pbCur++ = RTNET_DHCP_OPT_OPTION_OVERLOAD;
            *m_pbCur++ = fField;
            m_pfOverload = m_pbCur;
            *m_pbCur++ = 1;     /* bp_file flag */
        }
        else
            *m_pfOverload |= fField;

        /* pad current option field */
        while (m_pbCur != m_pbEnd)
            *m_pbCur++ = RTNET_DHCP_OPT_PAD; /** @todo not sure if this stuff is at all correct... */

        /* switch */
        m_pbCur = pbNew;
        m_pbEnd = pbNewEnd;
        return true;
    }

    /**
     * Begin an option.
     *
     * @returns true on success, false if we're out of space.
     *
     * @param   uOption     The option number.
     * @param   cb          The amount of data.
     */
    bool begin(uint8_t uOption, size_t cb)
    {
        /* Check that the data of the previous option has all been written. */
        Assert(   !m_pOpt
               || (m_pbCur - m_pOpt->dhcp_len == (uint8_t *)(m_pOpt + 1)));
        AssertMsg(cb <= 255, ("%#x\n", cb));

        /* Check if we need to overload more stuff. */
        if ((uintptr_t)(m_pbEnd - m_pbCur) < cb + 2 + (m_pfOverload ? 1 : 3))
        {
            m_pOpt = NULL;
            if (!overloadMore())
            {
                m_fOverflowed = true;
                AssertMsgFailedReturn(("%u %#x\n", uOption, cb), false);
            }
            if ((uintptr_t)(m_pbEnd - m_pbCur) < cb + 2 + 1)
            {
                m_fOverflowed = true;
                AssertMsgFailedReturn(("%u %#x\n", uOption, cb), false);
            }
        }

        /* Emit the option header. */
        m_pOpt = (PRTNETDHCPOPT)m_pbCur;
        m_pOpt->dhcp_opt = uOption;
        m_pOpt->dhcp_len = (uint8_t)cb;
        m_pbCur += 2;
        return true;
    }

    /**
     * Puts option data.
     *
     * @param   pvData      The data.
     * @param   cb          The amount to put.
     */
    void put(void const *pvData, size_t cb)
    {
        Assert(m_pOpt || m_fOverflowed);
        if (RT_LIKELY(m_pOpt))
        {
            Assert((uintptr_t)m_pbCur - (uintptr_t)(m_pOpt + 1) + cb  <= (size_t)m_pOpt->dhcp_len);
            memcpy(m_pbCur, pvData, cb);
            m_pbCur += cb;
        }
    }

    /**
     * Puts an IPv4 Address.
     *
     * @param   IPv4Addr    The address.
     */
    void putIPv4Addr(RTNETADDRIPV4 IPv4Addr)
    {
        put(&IPv4Addr, 4);
    }

    /**
     * Adds an IPv4 address option.
     *
     * @returns true/false just like begin().
     *
     * @param   uOption     The option number.
     * @param   IPv4Addr    The address.
     */
    bool optIPv4Addr(uint8_t uOption, RTNETADDRIPV4 IPv4Addr)
    {
        if (!begin(uOption, 4))
            return false;
        putIPv4Addr(IPv4Addr);
        return true;
    }

    /**
     * Adds an option taking 1 or more IPv4 address.
     *
     * If the vector contains no addresses, the option will not be added.
     *
     * @returns true/false just like begin().
     *
     * @param   uOption     The option number.
     * @param   rIPv4Addrs  Reference to the address vector.
     */
    bool optIPv4Addrs(uint8_t uOption, std::vector<RTNETADDRIPV4> const &rIPv4Addrs)
    {
        size_t const c = rIPv4Addrs.size();
        if (!c)
            return true;

        if (!begin(uOption, 4*c))
            return false;
        for (size_t i = 0; i < c; i++)
            putIPv4Addr(rIPv4Addrs[i]);
        return true;
    }

    /**
     * Puts an 8-bit integer.
     *
     * @param   u8          The integer.
     */
    void putU8(uint8_t u8)
    {
        put(&u8, 1);
    }

    /**
     * Adds an 8-bit integer option.
     *
     * @returns true/false just like begin().
     *
     * @param   uOption     The option number.
     * @param   u8          The integer
     */
    bool optU8(uint8_t uOption, uint8_t u8)
    {
        if (!begin(uOption, 1))
            return false;
        putU8(u8);
        return true;
    }

    /**
     * Puts an 32-bit integer (network endian).
     *
     * @param   u32         The integer.
     */
    void putU32(uint32_t u32)
    {
        put(&u32, 4);
    }

    /**
     * Adds an 32-bit integer (network endian) option.
     *
     * @returns true/false just like begin().
     *
     * @param   uOption     The option number.
     * @param   u32         The integer.
     */
    bool optU32(uint8_t uOption, uint32_t u32)
    {
        if (!begin(uOption, 4))
            return false;
        putU32(u32);
        return true;
    }

    /**
     * Puts a std::string.
     *
     * @param   rStr        Reference to the string.
     */
    void putStr(std::string const &rStr)
    {
        put(rStr.c_str(), rStr.size());
    }

    /**
     * Adds an std::string option if the string isn't empty.
     *
     * @returns true/false just like begin().
     *
     * @param   uOption     The option number.
     * @param   rStr        Reference to the string.
     */
    bool optStr(uint8_t uOption, std::string const &rStr)
    {
        const size_t cch = rStr.size();
        if (!cch)
            return true;

        if (!begin(uOption, cch))
            return false;
        put(rStr.c_str(), cch);
        return true;
    }

    /**
     * Whether we've overflowed.
     *
     * @returns true on overflow, false otherwise.
     */
    bool hasOverflowed(void) const
    {
        return m_fOverflowed;
    }

    /**
     * Adds the terminating END option.
     *
     * The END will always be added as we're reserving room for it, however, we
     * might have dropped previous options due to overflows and that is what the
     * return status indicates.
     *
     * @returns true on success, false on a (previous) overflow.
     */
    bool optEnd(void)
    {
        Assert((uintptr_t)(m_pbEnd - m_pbCur) < 4096);
        *m_pbCur++ = RTNET_DHCP_OPT_END;
        return !hasOverflowed();
    }
};

#endif
