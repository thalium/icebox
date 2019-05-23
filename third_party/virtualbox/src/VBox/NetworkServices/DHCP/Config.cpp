/* $Id: Config.cpp $ */
/** @file
 * Configuration for DHCP.
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


/**
 * XXX: license.
 */

#include <iprt/asm.h>
#include <iprt/getopt.h>
#include <iprt/net.h>
#include <iprt/time.h>

#include <VBox/sup.h>
#include <VBox/intnet.h>
#include <VBox/intnetinline.h>
#include <VBox/vmm/vmm.h>
#include <VBox/version.h>

#include <VBox/com/array.h>
#include <VBox/com/string.h>

#include <iprt/cpp/xml.h>

#define BASE_SERVICES_ONLY
#include "../NetLib/VBoxNetBaseService.h"
#include "../NetLib/VBoxNetLib.h"
#include "../NetLib/shared_ptr.h"

#include <list>
#include <vector>
#include <map>
#include <string>

#include "Config.h"
#include "ClientDataInt.h"

bool operator== (const Lease& lhs, const Lease& rhs)
{
    return (lhs.m.get() == rhs.m.get());
}


bool operator!= (const Lease& lhs, const Lease& rhs)
{
    return !(lhs == rhs);
}


bool operator< (const Lease& lhs, const Lease& rhs)
{
    return (   (lhs.getAddress() < rhs.getAddress())
            || (lhs.issued() < rhs.issued()));
}
/* consts */

const NullConfigEntity *g_NullConfig = new NullConfigEntity();
RootConfigEntity *g_RootConfig = new RootConfigEntity(std::string("ROOT"), 1200 /* 20 min. */);
const ClientMatchCriteria *g_AnyClient = new AnyClientMatchCriteria();

static ConfigurationManager *g_ConfigurationManager = ConfigurationManager::getConfigurationManager();

NetworkManager *NetworkManager::g_NetworkManager;

bool MACClientMatchCriteria::check(const Client& client) const
{
    return (client == m_mac);
}


int BaseConfigEntity::match(Client& client, BaseConfigEntity **cfg)
{
    int iMatch = (m_criteria && m_criteria->check(client) ? m_MatchLevel : 0);
    if (m_children.empty())
    {
        if (iMatch > 0)
        {
            *cfg = this;
            return iMatch;
        }
    }
    else
    {
        *cfg = this;
        /* XXX: hack */
        BaseConfigEntity *matching = this;
        int matchingLevel = m_MatchLevel;

        for (std::vector<BaseConfigEntity *>::iterator it = m_children.begin();
             it != m_children.end();
             ++it)
        {
            iMatch = (*it)->match(client, &matching);
            if (iMatch > matchingLevel)
            {
                *cfg = matching;
                matchingLevel = iMatch;
            }
        }
        return matchingLevel;
    }
    return iMatch;
}

/* Client */
/* Configs
    NetworkConfigEntity(std::string name,
                        ConfigEntity* pCfg,
                        ClientMatchCriteria* criteria,
                        RTNETADDRIPV4& networkID,
                        RTNETADDRIPV4& networkMask)
*/
static const RTNETADDRIPV4 g_AnyIpv4 = {0};
static const RTNETADDRIPV4 g_AllIpv4 = {0xffffffff};
RootConfigEntity::RootConfigEntity(std::string name, uint32_t expPeriod):
  NetworkConfigEntity(name, g_NullConfig, g_AnyClient, g_AnyIpv4, g_AllIpv4)
{
    m_MatchLevel = 2;
    m_u32ExpirationPeriod = expPeriod;
}

/* Configuration Manager */
struct ConfigurationManager::Data
{
    Data():fFileExists(false){}

    MapLease2Ip4Address  m_allocations;
    Ipv4AddressContainer m_nameservers;
    Ipv4AddressContainer m_routers;

    std::string          m_domainName;
    VecClient            m_clients;
    com::Utf8Str         m_leaseStorageFilename;
    bool                 fFileExists;
};

ConfigurationManager *ConfigurationManager::getConfigurationManager()
{
    if (!g_ConfigurationManager)


    {
        g_ConfigurationManager = new ConfigurationManager();
        g_ConfigurationManager->init();
    }

    return g_ConfigurationManager;
}


const std::string tagXMLLeases = "Leases";
const std::string tagXMLLeasesAttributeVersion = "version";
const std::string tagXMLLeasesVersion_1_0 = "1.0";
const std::string tagXMLLease = "Lease";
const std::string tagXMLLeaseAttributeMac = "mac";
const std::string tagXMLLeaseAttributeNetwork = "network";
const std::string tagXMLLeaseAddress = "Address";
const std::string tagXMLAddressAttributeValue = "value";
const std::string tagXMLLeaseTime = "Time";
const std::string tagXMLTimeAttributeIssued = "issued";
const std::string tagXMLTimeAttributeExpiration = "expiration";
const std::string tagXMLLeaseOptions = "Options";

/**
 * @verbatim
   <Leases version="1.0">
     <Lease mac="" network=""/>
      <Address value=""/>
      <Time issued="" expiration=""/>
      <options>
        <option name="" type=""/>
        </option>
      </options>
     </Lease>
   </Leases>
   @endverbatim
 */
int ConfigurationManager::loadFromFile(const com::Utf8Str& leaseStorageFileName)
{
    m->m_leaseStorageFilename = leaseStorageFileName;

    xml::XmlFileParser parser;
    xml::Document doc;

    try {
        parser.read(m->m_leaseStorageFilename.c_str(), doc);
    }
    catch (...)
    {
        return VINF_SUCCESS;
    }

    /* XML parsing */
    xml::ElementNode *root = doc.getRootElement();

    if (!root || !root->nameEquals(tagXMLLeases.c_str()))
    {
        m->fFileExists = false;
        return VERR_NOT_FOUND;
    }

    com::Utf8Str version;
    if (root)
        root->getAttributeValue(tagXMLLeasesAttributeVersion.c_str(), version);

    /* XXX: version check */
    xml::NodesLoop leases(*root);

    const xml::ElementNode *lease;
    while ((lease = leases.forAllNodes()))
    {
        if (!lease->nameEquals(tagXMLLease.c_str()))
            continue;

        ClientData *data = new ClientData();
        Lease l(data);
        if (l.fromXML(lease))
        {

            m->m_allocations.insert(MapLease2Ip4AddressPair(l, l.getAddress()));


            NetworkConfigEntity *pNetCfg = NULL;
            Client c(data);
            int rc = g_RootConfig->match(c, (BaseConfigEntity **)&pNetCfg);
            Assert(rc >= 0 && pNetCfg); RT_NOREF(rc);

            l.setConfig(pNetCfg);

            m->m_clients.push_back(c);
        }
    }

    return VINF_SUCCESS;
}


int ConfigurationManager::saveToFile()
{
    if (m->m_leaseStorageFilename.isEmpty())
        return VINF_SUCCESS;

    xml::Document doc;

    xml::ElementNode *root = doc.createRootElement(tagXMLLeases.c_str());
    if (!root)
        return VERR_INTERNAL_ERROR;

    root->setAttribute(tagXMLLeasesAttributeVersion.c_str(), tagXMLLeasesVersion_1_0.c_str());

    for(MapLease2Ip4AddressConstIterator it = m->m_allocations.begin();
        it != m->m_allocations.end(); ++it)
    {
        xml::ElementNode *lease = root->createChild(tagXMLLease.c_str());
        if (!it->first.toXML(lease))
        {
            /* XXX: todo logging + error handling */
        }
    }

    try {
        xml::XmlFileWriter writer(doc);
        writer.write(m->m_leaseStorageFilename.c_str(), true);
    } catch(...){}

    return VINF_SUCCESS;
}


int ConfigurationManager::extractRequestList(PCRTNETBOOTP pDhcpMsg, size_t cbDhcpMsg, RawOption& rawOpt)
{
    return ConfigurationManager::findOption(RTNET_DHCP_OPT_PARAM_REQ_LIST, pDhcpMsg, cbDhcpMsg, rawOpt);
}


Client ConfigurationManager::getClientByDhcpPacket(const RTNETBOOTP *pDhcpMsg, size_t cbDhcpMsg)
{

    VecClientIterator it;
    bool fDhcpValid = false;
    uint8_t uMsgType = 0;

    fDhcpValid = RTNetIPv4IsDHCPValid(NULL, pDhcpMsg, cbDhcpMsg, &uMsgType);
    AssertReturn(fDhcpValid, Client::NullClient);

    LogFlowFunc(("dhcp:mac:%RTmac\n", &pDhcpMsg->bp_chaddr.Mac));
    /* 1st. client IDs */
    for ( it = m->m_clients.begin();
         it != m->m_clients.end();
         ++it)
    {
        if ((*it) == pDhcpMsg->bp_chaddr.Mac)
        {
            LogFlowFunc(("client:mac:%RTmac\n",  it->getMacAddress()));
            /* check timestamp that request wasn't expired. */
            return (*it);
        }
    }

    if (it == m->m_clients.end())
    {
        /* We hasn't got any session for this client */
        Client c;
        c.initWithMac(pDhcpMsg->bp_chaddr.Mac);
        m->m_clients.push_back(c);
        return m->m_clients.back();
    }

    return Client::NullClient;
}

/**
 * Finds an option.
 *
 * @returns On success, a pointer to the first byte in the option data (no none
 *          then it'll be the byte following the 0 size field) and *pcbOpt set
 *          to the option length.
 *          On failure, NULL is returned and *pcbOpt unchanged.
 *
 * @param   uOption     The option to search for.
 * @param   pDhcpMsg    The DHCP message.
 *                      that this is adjusted if the option length is larger
 *                      than the message buffer.
 * @param   cbDhcpMsg   Size of the DHCP message.
 * @param   opt         The actual option we found.
 */
int
ConfigurationManager::findOption(uint8_t uOption, PCRTNETBOOTP pDhcpMsg, size_t cbDhcpMsg, RawOption& opt)
{
    Assert(uOption != RTNET_DHCP_OPT_PAD);
    Assert(uOption != RTNET_DHCP_OPT_END);

    /*
     * Validate the DHCP bits and figure the max size of the options in the vendor field.
     */
    if (cbDhcpMsg <= RT_UOFFSETOF(RTNETBOOTP, bp_vend.Dhcp.dhcp_opts))
        return VERR_INVALID_PARAMETER;

    if (pDhcpMsg->bp_vend.Dhcp.dhcp_cookie != RT_H2N_U32_C(RTNET_DHCP_COOKIE))
        return VERR_INVALID_PARAMETER;

    size_t cbLeft = cbDhcpMsg - RT_UOFFSETOF(RTNETBOOTP, bp_vend.Dhcp.dhcp_opts);
    if (cbLeft > RTNET_DHCP_OPT_SIZE)
        cbLeft = RTNET_DHCP_OPT_SIZE;

    /*
     * Search the vendor field.
     */
    uint8_t const  *pb = &pDhcpMsg->bp_vend.Dhcp.dhcp_opts[0];
    while (pb && cbLeft > 0)
    {
        uint8_t uCur  = *pb;
        if (uCur == RTNET_DHCP_OPT_PAD)
        {
            cbLeft--;
            pb++;
        }
        else if (uCur == RTNET_DHCP_OPT_END)
            break;
        else if (cbLeft <= 1)
            break;
        else
        {
            uint8_t cbCur = pb[1];
            if (cbCur > cbLeft - 2)
                cbCur = (uint8_t)(cbLeft - 2);
            if (uCur == uOption)
            {
                opt.u8OptId = uCur;
                memcpy(opt.au8RawOpt, pb+2, cbCur);
                opt.cbRawOpt = cbCur;
                return VINF_SUCCESS;
            }
            pb     += cbCur + 2;
            cbLeft -= cbCur + 2;
        }
    }

    /** @todo search extended dhcp option field(s) when present */

    return VERR_NOT_FOUND;
}


/**
 * We bind lease for client till it continue with it on DHCPREQUEST.
 */
Lease ConfigurationManager::allocateLease4Client(const Client& client, PCRTNETBOOTP pDhcpMsg, size_t cbDhcpMsg)
{
    {
        /**
         * This mean that client has already bound or commited lease.
         * If we've it happens it means that we received DHCPDISCOVER twice.
         */
        const Lease l = client.lease();
        if (l != Lease::NullLease)
        {
            /* Here we should take lease from the m_allocation which was feed with leases
             *  on start
             */
            if (l.isExpired())
            {
                expireLease4Client(const_cast<Client&>(client));
                if (!l.isExpired())
                    return l;
            }
            else
            {
                AssertReturn(l.getAddress().u != 0, Lease::NullLease);
                return l;
            }
        }
    }

    RTNETADDRIPV4 hintAddress;
    RawOption opt;
    NetworkConfigEntity *pNetCfg;

    Client cl(client);
    AssertReturn(g_RootConfig->match(cl, (BaseConfigEntity **)&pNetCfg) > 0, Lease::NullLease);

    /* DHCPDISCOVER MAY contain request address */
    hintAddress.u = 0;
    int rc = findOption(RTNET_DHCP_OPT_REQ_ADDR, pDhcpMsg, cbDhcpMsg, opt);
    if (RT_SUCCESS(rc))
    {
        hintAddress.u = *(uint32_t *)opt.au8RawOpt;
        if (   RT_H2N_U32(hintAddress.u) < RT_H2N_U32(pNetCfg->lowerIp().u)
            || RT_H2N_U32(hintAddress.u) > RT_H2N_U32(pNetCfg->upperIp().u))
            hintAddress.u = 0; /* clear hint */
    }

    if (   hintAddress.u
        && !isAddressTaken(hintAddress))
    {
        Lease l(cl);
        l.setConfig(pNetCfg);
        l.setAddress(hintAddress);
        m->m_allocations.insert(MapLease2Ip4AddressPair(l, hintAddress));
        return l;
    }

    uint32_t u32 = 0;
    for(u32 = RT_H2N_U32(pNetCfg->lowerIp().u);
        u32 <= RT_H2N_U32(pNetCfg->upperIp().u);
        ++u32)
    {
        RTNETADDRIPV4 address;
        address.u = RT_H2N_U32(u32);
        if (!isAddressTaken(address))
        {
            Lease l(cl);
            l.setConfig(pNetCfg);
            l.setAddress(address);
            m->m_allocations.insert(MapLease2Ip4AddressPair(l, address));
            return l;
        }
    }

    return Lease::NullLease;
}


int ConfigurationManager::commitLease4Client(Client& client)
{
    Lease l = client.lease();
    AssertReturn(l != Lease::NullLease, VERR_INTERNAL_ERROR);

    l.bindingPhase(false);
    const NetworkConfigEntity *pCfg = l.getConfig();

    AssertPtr(pCfg);
    l.setExpiration(pCfg->expirationPeriod());
    l.phaseStart(RTTimeMilliTS());

    saveToFile();

    return VINF_SUCCESS;
}


int ConfigurationManager::expireLease4Client(Client& client)
{
    Lease l = client.lease();
    AssertReturn(l != Lease::NullLease, VERR_INTERNAL_ERROR);

    if (l.isInBindingPhase())
    {

        MapLease2Ip4AddressIterator it = m->m_allocations.find(l);
        AssertReturn(it != m->m_allocations.end(), VERR_NOT_FOUND);

        /*
         * XXX: perhaps it better to keep this allocation ????
         */
        m->m_allocations.erase(it);

        l.expire();
        return VINF_SUCCESS;
    }

    l = Lease(client); /* re-new */
    return VINF_SUCCESS;
}


bool ConfigurationManager::isAddressTaken(const RTNETADDRIPV4& addr, Lease& lease)
{
    MapLease2Ip4AddressIterator it;

    for (it = m->m_allocations.begin();
         it != m->m_allocations.end();
         ++it)
    {
        if (it->second.u == addr.u)
        {
            if (lease != Lease::NullLease)
                lease = it->first;

            return true;
        }
    }
    lease = Lease::NullLease;
    return false;
}


bool ConfigurationManager::isAddressTaken(const RTNETADDRIPV4& addr)
{
    Lease ignore;
    return isAddressTaken(addr, ignore);
}


NetworkConfigEntity *ConfigurationManager::addNetwork(NetworkConfigEntity *,
                                    const RTNETADDRIPV4& networkId,
                                    const RTNETADDRIPV4& netmask,
                                    RTNETADDRIPV4& LowerAddress,
                                    RTNETADDRIPV4& UpperAddress)
{
        static int id;
        char name[64];

        RTStrPrintf(name, RT_ELEMENTS(name), "network-%d", id);
        std::string strname(name);
        id++;


        if (!LowerAddress.u)
            LowerAddress = networkId;

        if (!UpperAddress.u)
            UpperAddress.u = networkId.u | (~netmask.u);

        return new NetworkConfigEntity(strname,
                            g_RootConfig,
                            g_AnyClient,
                            5,
                            networkId,
                            netmask,
                            LowerAddress,
                            UpperAddress);
}

HostConfigEntity *ConfigurationManager::addHost(NetworkConfigEntity* pCfg,
                                                const RTNETADDRIPV4& address,
                                                ClientMatchCriteria *criteria)
{
    static int id;
    char name[64];

    RTStrPrintf(name, RT_ELEMENTS(name), "host-%d", id);
    std::string strname(name);
    id++;

    return new HostConfigEntity(address, strname, pCfg, criteria);
}

int ConfigurationManager::addToAddressList(uint8_t u8OptId, RTNETADDRIPV4& address)
{
    switch(u8OptId)
    {
        case RTNET_DHCP_OPT_DNS:
            m->m_nameservers.push_back(address);
            break;
        case RTNET_DHCP_OPT_ROUTERS:
            m->m_routers.push_back(address);
            break;
        default:
            Log(("dhcp-opt: list (%d) unsupported\n", u8OptId));
    }
    return VINF_SUCCESS;
}


int ConfigurationManager::flushAddressList(uint8_t u8OptId)
{
    switch(u8OptId)
    {
        case RTNET_DHCP_OPT_DNS:
            m->m_nameservers.clear();
            break;
        case RTNET_DHCP_OPT_ROUTERS:
            m->m_routers.clear();
            break;
        default:
            Log(("dhcp-opt: list (%d) unsupported\n", u8OptId));
    }
    return VINF_SUCCESS;
}


const Ipv4AddressContainer& ConfigurationManager::getAddressList(uint8_t u8OptId)
{
    switch(u8OptId)
    {
        case RTNET_DHCP_OPT_DNS:
            return m->m_nameservers;

        case RTNET_DHCP_OPT_ROUTERS:
            return m->m_routers;

    }
    /* XXX: Grrr !!! */
    return m_empty;
}


int ConfigurationManager::setString(uint8_t u8OptId, const std::string& str)
{
    switch (u8OptId)
    {
        case RTNET_DHCP_OPT_DOMAIN_NAME:
            m->m_domainName = str;
            break;
        default:
            break;
    }

    return VINF_SUCCESS;
}


const std::string &ConfigurationManager::getString(uint8_t u8OptId)
{
    switch (u8OptId)
    {
        case RTNET_DHCP_OPT_DOMAIN_NAME:
            if (m->m_domainName.length())
                return m->m_domainName;
            return m_noString;
        default:
            break;
    }

    return m_noString;
}


void ConfigurationManager::init()
{
    m = new ConfigurationManager::Data();
}


ConfigurationManager::~ConfigurationManager() { if (m) delete m; }

/**
 * Network manager
 */
struct NetworkManager::Data
{
    Data()
    {
        RT_ZERO(BootPReplyMsg);
        cbBooPReplyMsg = 0;

        m_OurAddress.u = 0;
        m_OurNetmask.u = 0;
        RT_ZERO(m_OurMac);
    }

    union {
        RTNETBOOTP BootPHeader;
        uint8_t au8Storage[1024];
    } BootPReplyMsg;
    int cbBooPReplyMsg;

    RTNETADDRIPV4 m_OurAddress;
    RTNETADDRIPV4 m_OurNetmask;
    RTMAC m_OurMac;

    ComPtr<IDHCPServer>  m_DhcpServer;
    const VBoxNetHlpUDPService *m_service;
};


NetworkManager::NetworkManager():m(NULL)
{
    m = new NetworkManager::Data();
}


NetworkManager::~NetworkManager()
{
    delete m;
    m = NULL;
}


NetworkManager *NetworkManager::getNetworkManager(ComPtr<IDHCPServer> aDhcpServer)
{
    if (!g_NetworkManager)
    {
        g_NetworkManager = new NetworkManager();
        g_NetworkManager->m->m_DhcpServer = aDhcpServer;
    }

    return g_NetworkManager;
}


const RTNETADDRIPV4& NetworkManager::getOurAddress() const
{
    return m->m_OurAddress;
}


const RTNETADDRIPV4& NetworkManager::getOurNetmask() const
{
    return m->m_OurNetmask;
}


const RTMAC& NetworkManager::getOurMac() const
{
    return m->m_OurMac;
}


void NetworkManager::setOurAddress(const RTNETADDRIPV4& aAddress)
{
    m->m_OurAddress = aAddress;
}


void NetworkManager::setOurNetmask(const RTNETADDRIPV4& aNetmask)
{
    m->m_OurNetmask = aNetmask;
}


void NetworkManager::setOurMac(const RTMAC& aMac)
{
    m->m_OurMac = aMac;
}


void NetworkManager::setService(const VBoxNetHlpUDPService *srv)
{
    m->m_service = srv;
}

/**
 * Network manager creates DHCPOFFER datagramm
 */
int NetworkManager::offer4Client(const Client& client, uint32_t u32Xid,
                                 uint8_t *pu8ReqList, int cReqList)
{
    Lease l(client); /* XXX: oh, it looks badly, but now we have lease */
    prepareReplyPacket4Client(client, u32Xid);

    RTNETADDRIPV4 address = l.getAddress();
    m->BootPReplyMsg.BootPHeader.bp_yiaddr =  address;

    /* Ubuntu ???*/
    m->BootPReplyMsg.BootPHeader.bp_ciaddr =  address;

    /* options:
     * - IP lease time
     * - message type
     * - server identifier
     */
    RawOption opt;
    RT_ZERO(opt);

    std::vector<RawOption> extra;
    opt.u8OptId = RTNET_DHCP_OPT_MSG_TYPE;
    opt.au8RawOpt[0] = RTNET_DHCP_MT_OFFER;
    opt.cbRawOpt = 1;
    extra.push_back(opt);

    opt.u8OptId = RTNET_DHCP_OPT_LEASE_TIME;

    const NetworkConfigEntity *pCfg = l.getConfig();
    AssertPtr(pCfg);

    *(uint32_t *)opt.au8RawOpt = RT_H2N_U32(pCfg->expirationPeriod());
    opt.cbRawOpt = sizeof(RTNETADDRIPV4);

    extra.push_back(opt);

    processParameterReqList(client, pu8ReqList, cReqList, extra);

    return doReply(client, extra);
}

/**
 * Network manager creates DHCPACK
 */
int NetworkManager::ack(const Client& client, uint32_t u32Xid,
                        uint8_t *pu8ReqList, int cReqList)
{
    RTNETADDRIPV4 address;

    prepareReplyPacket4Client(client, u32Xid);

    Lease l = client.lease();
    address = l.getAddress();
    m->BootPReplyMsg.BootPHeader.bp_ciaddr =  address;


    /* rfc2131 4.3.1 is about DHCPDISCOVER and this value is equal to ciaddr from
     * DHCPREQUEST or 0 ...
     * XXX: Using addressHint is not correct way to initialize [cy]iaddress...
     */
    m->BootPReplyMsg.BootPHeader.bp_ciaddr = address;
    m->BootPReplyMsg.BootPHeader.bp_yiaddr = address;

    Assert(m->BootPReplyMsg.BootPHeader.bp_yiaddr.u);

    /* options:
     * - IP address lease time (if DHCPREQUEST)
     * - message type
     * - server identifier
     */
    RawOption opt;
    RT_ZERO(opt);

    std::vector<RawOption> extra;
    opt.u8OptId = RTNET_DHCP_OPT_MSG_TYPE;
    opt.au8RawOpt[0] = RTNET_DHCP_MT_ACK;
    opt.cbRawOpt = 1;
    extra.push_back(opt);

    /*
     * XXX: lease time should be conditional. If on dhcprequest then tim should be provided,
     * else on dhcpinform it mustn't.
     */
    opt.u8OptId = RTNET_DHCP_OPT_LEASE_TIME;
    *(uint32_t *)opt.au8RawOpt = RT_H2N_U32(l.getExpiration());
    opt.cbRawOpt = sizeof(RTNETADDRIPV4);
    extra.push_back(opt);

    processParameterReqList(client, pu8ReqList, cReqList, extra);

    return doReply(client, extra);
}

/**
 * Network manager creates DHCPNAK
 */
int NetworkManager::nak(const Client& client, uint32_t u32Xid)
{

    Lease l = client.lease();
    if (l == Lease::NullLease)
        return VERR_INTERNAL_ERROR;

    prepareReplyPacket4Client(client, u32Xid);

    /* this field filed in prepareReplyPacket4Session, and
     * RFC 2131 require to have it zero fo NAK.
     */
    m->BootPReplyMsg.BootPHeader.bp_yiaddr.u = 0;

    /* options:
     * - message type (if DHCPREQUEST)
     * - server identifier
     */
    RawOption opt;
    std::vector<RawOption> extra;

    opt.u8OptId = RTNET_DHCP_OPT_MSG_TYPE;
    opt.au8RawOpt[0] = RTNET_DHCP_MT_NAC;
    opt.cbRawOpt = 1;
    extra.push_back(opt);

    return doReply(client, extra);
}

/**
 *
 */
int NetworkManager::prepareReplyPacket4Client(const Client& client, uint32_t u32Xid)
{
    RT_ZERO(m->BootPReplyMsg);

    m->BootPReplyMsg.BootPHeader.bp_op     = RTNETBOOTP_OP_REPLY;
    m->BootPReplyMsg.BootPHeader.bp_htype  = RTNET_ARP_ETHER;
    m->BootPReplyMsg.BootPHeader.bp_hlen   = sizeof(RTMAC);
    m->BootPReplyMsg.BootPHeader.bp_hops   = 0;
    m->BootPReplyMsg.BootPHeader.bp_xid    = u32Xid;
    m->BootPReplyMsg.BootPHeader.bp_secs   = 0;
    /* XXX: bp_flags should be processed specially */
    m->BootPReplyMsg.BootPHeader.bp_flags  = 0;
    m->BootPReplyMsg.BootPHeader.bp_ciaddr.u = 0;
    m->BootPReplyMsg.BootPHeader.bp_giaddr.u = 0;

    m->BootPReplyMsg.BootPHeader.bp_chaddr.Mac = client.getMacAddress();

    const Lease l = client.lease();
    m->BootPReplyMsg.BootPHeader.bp_yiaddr = l.getAddress();
    m->BootPReplyMsg.BootPHeader.bp_siaddr.u = 0;


    m->BootPReplyMsg.BootPHeader.bp_vend.Dhcp.dhcp_cookie = RT_H2N_U32_C(RTNET_DHCP_COOKIE);

    memset(&m->BootPReplyMsg.BootPHeader.bp_vend.Dhcp.dhcp_opts[0],
           '\0',
           RTNET_DHCP_OPT_SIZE);

    return VINF_SUCCESS;
}


int NetworkManager::doReply(const Client& client, const std::vector<RawOption>& extra)
{
    int rc;

    /*
      Options....
     */
    VBoxNetDhcpWriteCursor Cursor(&m->BootPReplyMsg.BootPHeader, RTNET_DHCP_NORMAL_SIZE);

    /* The basics */

    Cursor.optIPv4Addr(RTNET_DHCP_OPT_SERVER_ID, m->m_OurAddress);

    const Lease l = client.lease();
    const std::map<uint8_t, RawOption>& options = l.options();

    for(std::vector<RawOption>::const_iterator it = extra.begin();
        it != extra.end(); ++it)
    {
        if (!Cursor.begin(it->u8OptId, it->cbRawOpt))
            break;
        Cursor.put(it->au8RawOpt, it->cbRawOpt);

    }

    for(std::map<uint8_t, RawOption>::const_iterator it = options.begin();
        it != options.end(); ++it)
    {
        if (!Cursor.begin(it->second.u8OptId, it->second.cbRawOpt))
            break;
        Cursor.put(it->second.au8RawOpt, it->second.cbRawOpt);

    }

    Cursor.optEnd();

    /*
     */
#if 0
    /** @todo need to see someone set this flag to check that it's correct. */
    if (!(pDhcpMsg->bp_flags & RTNET_DHCP_FLAGS_NO_BROADCAST))
    {
        rc = VBoxNetUDPUnicast(m_pSession,
                               m_hIf,
                               m_pIfBuf,
                               m_OurAddress,
                               &m_OurMac,
                               RTNETIPV4_PORT_BOOTPS,                 /* sender */
                               IPv4AddrBrdCast,
                               &BootPReplyMsg.BootPHeader->bp_chaddr.Mac,
                               RTNETIPV4_PORT_BOOTPC,    /* receiver */
                               &BootPReplyMsg, cbBooPReplyMsg);
    }
    else
#endif
        rc = m->m_service->hlpUDPBroadcast(RTNETIPV4_PORT_BOOTPS,               /* sender */
                                           RTNETIPV4_PORT_BOOTPC,
                                           &m->BootPReplyMsg,
                                           RTNET_DHCP_NORMAL_SIZE);

    AssertRCReturn(rc,rc);

    return VINF_SUCCESS;
}


/*
 * XXX: TODO: Share decoding code with DHCPServer::addOption.
 */
static int parseDhcpOptionText(const char *pszText,
                               int *pOptCode, char **ppszOptText, int *pOptEncoding)
{
    uint8_t u8Code;
    uint32_t u32Enc;
    char *pszNext;
    int rc;

    rc = RTStrToUInt8Ex(pszText, &pszNext, 10, &u8Code);
    if (!RT_SUCCESS(rc))
        return VERR_PARSE_ERROR;

    switch (*pszNext)
    {
        case ':':           /* support legacy format too */
        {
            u32Enc = 0;
            break;
        }

        case '=':
        {
            u32Enc = 1;
            break;
        }

        case '@':
        {
            rc = RTStrToUInt32Ex(pszNext + 1, &pszNext, 10, &u32Enc);
            if (!RT_SUCCESS(rc))
                return VERR_PARSE_ERROR;
            if (*pszNext != '=')
                return VERR_PARSE_ERROR;
            break;
        }

        default:
            return VERR_PARSE_ERROR;
    }

    *pOptCode = u8Code;
    *ppszOptText = pszNext + 1;
    *pOptEncoding = (int)u32Enc;

    return VINF_SUCCESS;
}


static int fillDhcpOption(RawOption &opt, const std::string &OptText, int OptEncoding)
{
    int rc;

    if (OptEncoding == DhcpOptEncoding_Hex)
    {
        if (OptText.empty())
            return VERR_INVALID_PARAMETER;

        size_t cbRawOpt = 0;
        char *pszNext = const_cast<char *>(OptText.c_str());
        while (*pszNext != '\0')
        {
            if (cbRawOpt >= RT_ELEMENTS(opt.au8RawOpt))
                return VERR_INVALID_PARAMETER;

            uint8_t u8Byte;
            rc = RTStrToUInt8Ex(pszNext, &pszNext, 16, &u8Byte);
            if (!RT_SUCCESS(rc))
                return rc;

            if (*pszNext == ':')
                ++pszNext;
            else if (*pszNext != '\0')
                return VERR_PARSE_ERROR;

            opt.au8RawOpt[cbRawOpt] = u8Byte;
            ++cbRawOpt;
        }
        opt.cbRawOpt = (uint8_t)cbRawOpt;
    }
    else if (OptEncoding == DhcpOptEncoding_Legacy)
    {
        /*
         * XXX: TODO: encode "known" option opt.u8OptId
         */
        return VERR_INVALID_PARAMETER;
    }

    return VINF_SUCCESS;
}


int NetworkManager::processParameterReqList(const Client& client, const uint8_t *pu8ReqList,
                                            int cReqList, std::vector<RawOption>& extra)
{
    int rc;

    const Lease l = client.lease();

    const NetworkConfigEntity *pNetCfg = l.getConfig();

    /*
     * XXX: Brute-force.  Unfortunately, there's no notification event
     * for changes.  Should at least cache the options for a short
     * time, enough to last discover/offer/request/ack cycle.
     */
    typedef std::map< int, std::pair<std::string, int> > DhcpOptionMap;
    DhcpOptionMap OptMap;

    if (!m->m_DhcpServer.isNull())
    {
        com::SafeArray<BSTR> strings;
        com::Bstr str;
        HRESULT hrc;
        int OptCode, OptEncoding;
        char *pszOptText;

        strings.setNull();
        hrc = m->m_DhcpServer->COMGETTER(GlobalOptions)(ComSafeArrayAsOutParam(strings));
        AssertComRC(hrc);
        for (size_t i = 0; i < strings.size(); ++i)
        {
            com::Utf8Str encoded(strings[i]);
            rc = parseDhcpOptionText(encoded.c_str(),
                                     &OptCode, &pszOptText, &OptEncoding);
            if (!RT_SUCCESS(rc))
                continue;

            OptMap[OptCode] = std::make_pair(pszOptText, OptEncoding);
        }

        const RTMAC &mac = client.getMacAddress();
        char strMac[6*2+1] = "";
        RTStrPrintf(strMac, sizeof(strMac), "%02x%02x%02x%02x%02x%02x",
                    mac.au8[0], mac.au8[1], mac.au8[2],
                    mac.au8[3], mac.au8[4], mac.au8[5]);

        strings.setNull();
        hrc = m->m_DhcpServer->GetMacOptions(com::Bstr(strMac).raw(),
                                             ComSafeArrayAsOutParam(strings));
        AssertComRC(hrc);
        for (size_t i = 0; i < strings.size(); ++i)
        {
            com::Utf8Str text(strings[i]);
            rc = parseDhcpOptionText(text.c_str(),
                                     &OptCode, &pszOptText, &OptEncoding);
            if (!RT_SUCCESS(rc))
                continue;

            OptMap[OptCode] = std::make_pair(pszOptText, OptEncoding);
        }
    }

    /* request parameter list */
    RawOption opt;
    bool fIgnore;
    uint8_t u8Req;
    for (int idxParam = 0; idxParam < cReqList; ++idxParam)
    {
        fIgnore = false;
        RT_ZERO(opt);
        u8Req = opt.u8OptId = pu8ReqList[idxParam];

        switch(u8Req)
        {
            case RTNET_DHCP_OPT_SUBNET_MASK:
                ((PRTNETADDRIPV4)opt.au8RawOpt)->u = pNetCfg->netmask().u;
                opt.cbRawOpt = sizeof(RTNETADDRIPV4);

                break;

            case RTNET_DHCP_OPT_ROUTERS:
            case RTNET_DHCP_OPT_DNS:
                {
                    const Ipv4AddressContainer lst =
                      g_ConfigurationManager->getAddressList(u8Req);
                    PRTNETADDRIPV4 pAddresses = (PRTNETADDRIPV4)&opt.au8RawOpt[0];

                    for (Ipv4AddressConstIterator it = lst.begin();
                         it != lst.end();
                         ++it)
                    {
                        *pAddresses = (*it);
                        pAddresses++;
                        opt.cbRawOpt += sizeof(RTNETADDRIPV4);
                    }

                    if (lst.empty())
                        fIgnore = true;
                }
                break;
            case RTNET_DHCP_OPT_DOMAIN_NAME:
                {
                    std::string domainName = g_ConfigurationManager->getString(u8Req);
                    if (domainName == g_ConfigurationManager->m_noString)
                    {
                        fIgnore = true;
                        break;
                    }

                    size_t cchLength = domainName.length();
                    if (cchLength >= sizeof(opt.au8RawOpt))
                        cchLength = sizeof(opt.au8RawOpt) - 1;
                    memcpy(&opt.au8RawOpt[0], domainName.c_str(), cchLength);
                    opt.au8RawOpt[cchLength] = '\0';
                    opt.cbRawOpt = (uint8_t)cchLength;
                }
                break;
            default:
                {
                    DhcpOptionMap::const_iterator it = OptMap.find((int)u8Req);
                    if (it == OptMap.end())
                    {
                        Log(("opt: %d is ignored\n", u8Req));
                        fIgnore = true;
                    }
                    else
                    {
                        std::string OptText((*it).second.first);
                        int OptEncoding((*it).second.second);

                        rc = fillDhcpOption(opt, OptText, OptEncoding);
                        if (!RT_SUCCESS(rc))
                        {
                            fIgnore = true;
                            break;
                        }
                    }
                }
                break;
        }

        if (!fIgnore)
            extra.push_back(opt);

    }

    return VINF_SUCCESS;
}

/* Client */
Client::Client()
{
    m = SharedPtr<ClientData>();
}


void Client::initWithMac(const RTMAC& mac)
{
    m = SharedPtr<ClientData>(new ClientData());
    m->m_mac = mac;
}


bool Client::operator== (const RTMAC& mac) const
{
    return (m.get() && m->m_mac == mac);
}


const RTMAC& Client::getMacAddress() const
{
    return m->m_mac;
}


Lease Client::lease()
{
    if (!m.get()) return Lease::NullLease;

    if (m->fHasLease)
        return Lease(*this);
    else
        return Lease::NullLease;
}


const Lease Client::lease() const
{
    return const_cast<Client *>(this)->lease();
}


Client::Client(ClientData *data):m(SharedPtr<ClientData>(data)){}

/* Lease */
Lease::Lease()
{
    m = SharedPtr<ClientData>();
}


Lease::Lease (const Client& c)
{
    m = SharedPtr<ClientData>(c.m);
    if (   !m->fHasLease
        || (   isExpired()
            && !isInBindingPhase()))
    {
        m->fHasLease = true;
        m->fBinding = true;
        phaseStart(RTTimeMilliTS());
    }
}


bool Lease::isExpired() const
{
    AssertPtrReturn(m.get(), false);

    if (!m->fBinding)
        return (ASMDivU64ByU32RetU32(RTTimeMilliTS() - m->u64TimestampLeasingStarted, 1000)
                > m->u32LeaseExpirationPeriod);
    else
        return (ASMDivU64ByU32RetU32(RTTimeMilliTS() - m->u64TimestampBindingStarted, 1000)
                > m->u32BindExpirationPeriod);
}


void Lease::expire()
{
    /* XXX: TODO */
}


void Lease::phaseStart(uint64_t u64Start)
{
    if (m->fBinding)
        m->u64TimestampBindingStarted = u64Start;
    else
        m->u64TimestampLeasingStarted = u64Start;
}


void Lease::bindingPhase(bool fOnOff)
{
    m->fBinding = fOnOff;
}


bool Lease::isInBindingPhase() const
{
    return m->fBinding;
}


uint64_t Lease::issued() const
{
    return m->u64TimestampLeasingStarted;
}


void Lease::setExpiration(uint32_t exp)
{
    if (m->fBinding)
        m->u32BindExpirationPeriod = exp;
    else
        m->u32LeaseExpirationPeriod = exp;
}


uint32_t Lease::getExpiration() const
{
    if (m->fBinding)
        return m->u32BindExpirationPeriod;
    else
        return m->u32LeaseExpirationPeriod;
}


RTNETADDRIPV4 Lease::getAddress() const
{
    return m->m_address;
}


void Lease::setAddress(RTNETADDRIPV4 address)
{
    m->m_address = address;
}


const NetworkConfigEntity *Lease::getConfig() const
{
    return m->pCfg;
}


void Lease::setConfig(NetworkConfigEntity *pCfg)
{
    m->pCfg = pCfg;
}


const MapOptionId2RawOption& Lease::options() const
{
    return m->options;
}


Lease::Lease(ClientData *pd):m(SharedPtr<ClientData>(pd)){}


bool Lease::toXML(xml::ElementNode *node) const
{
    xml::AttributeNode *pAttribNode = node->setAttribute(tagXMLLeaseAttributeMac.c_str(),
                                                         com::Utf8StrFmt("%RTmac", &m->m_mac));
    if (!pAttribNode)
        return false;

    pAttribNode = node->setAttribute(tagXMLLeaseAttributeNetwork.c_str(),
                                     com::Utf8StrFmt("%RTnaipv4", m->m_network));
    if (!pAttribNode)
        return false;

    xml::ElementNode *pLeaseAddress = node->createChild(tagXMLLeaseAddress.c_str());
    if (!pLeaseAddress)
        return false;

    pAttribNode = pLeaseAddress->setAttribute(tagXMLAddressAttributeValue.c_str(),
                                              com::Utf8StrFmt("%RTnaipv4", m->m_address));
    if (!pAttribNode)
        return false;

    xml::ElementNode *pLeaseTime = node->createChild(tagXMLLeaseTime.c_str());
    if (!pLeaseTime)
        return false;

    pAttribNode = pLeaseTime->setAttribute(tagXMLTimeAttributeIssued.c_str(),
                                           m->u64TimestampLeasingStarted);
    if (!pAttribNode)
        return false;

    pAttribNode = pLeaseTime->setAttribute(tagXMLTimeAttributeExpiration.c_str(),
                                           m->u32LeaseExpirationPeriod);
    if (!pAttribNode)
        return false;

    return true;
}


bool Lease::fromXML(const xml::ElementNode *node)
{
    com::Utf8Str mac;
    bool valueExists = node->getAttributeValue(tagXMLLeaseAttributeMac.c_str(), mac);
    if (!valueExists) return false;
    int rc = RTNetStrToMacAddr(mac.c_str(), &m->m_mac);
    if (RT_FAILURE(rc)) return false;

    com::Utf8Str network;
    valueExists = node->getAttributeValue(tagXMLLeaseAttributeNetwork.c_str(), network);
    if (!valueExists) return false;
    rc = RTNetStrToIPv4Addr(network.c_str(), &m->m_network);
    if (RT_FAILURE(rc)) return false;

    /* Address */
    const xml::ElementNode *address = node->findChildElement(tagXMLLeaseAddress.c_str());
    if (!address) return false;
    com::Utf8Str addressValue;
    valueExists = address->getAttributeValue(tagXMLAddressAttributeValue.c_str(), addressValue);
    if (!valueExists) return false;
    rc = RTNetStrToIPv4Addr(addressValue.c_str(), &m->m_address);

    /* Time */
    const xml::ElementNode *time = node->findChildElement(tagXMLLeaseTime.c_str());
    if (!time) return false;

    valueExists = time->getAttributeValue(tagXMLTimeAttributeIssued.c_str(),
                                          &m->u64TimestampLeasingStarted);
    if (!valueExists) return false;
    m->fBinding = false;

    valueExists = time->getAttributeValue(tagXMLTimeAttributeExpiration.c_str(),
                                          &m->u32LeaseExpirationPeriod);
    if (!valueExists) return false;

    m->fHasLease = true;
    return true;
}


const Lease Lease::NullLease;

const Client Client::NullClient;
