// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/helpers/utils.hpp>
#include <formatters.hpp>

namespace app
{
namespace obmc
{
namespace entity
{

using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;
using namespace app::helpers::utils;

class CertificateBase : public entity::Collection, public CachedSource
{
  public:
    enum class Purpose
    {
        trustStore,
        ldap,
        https,
        unknown
    };

    enum class KeyUsage
    {
        digitalSignature,
        nonRepudiation,
        keyEncipherment,
        dataEncipherment,
        keyAgreement,
        keyCertSign,
        crlSigning,
        encipherOnly,
        decipherOnly,
        serverAuthentication,
        clientAuthentication,
        codeSigning,
        emailProtection,
        timestamping,
        ocspSigning
    };

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, CertificateString)
    ENTITY_DECL_FIELD(std::string, Issuer)

    ENTITY_DECL_FIELD(std::string, IssuerCity)
    ENTITY_DECL_FIELD(std::string, IssuerCommonName)
    ENTITY_DECL_FIELD(std::string, IssuerCountry)
    ENTITY_DECL_FIELD(std::string, IssuerDisplayString)
    ENTITY_DECL_FIELD(std::string, IssuerEmail)
    ENTITY_DECL_FIELD(std::string, IssuerOrganization)
    ENTITY_DECL_FIELD(std::string, IssuerOrganizationalUnit)
    ENTITY_DECL_FIELD(std::string, IssuerState)
    ENTITY_DECL_FIELD(std::string, IssuerAlgorithm)
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, IssuerAdditionalCommonNames,
                          {})
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, IssuerDomainComponents, {})
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>,
                          IssuerAdditionalOrganizationalUnits, {})
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, IssuerKeyUsage, {})

    ENTITY_DECL_FIELD(std::string, Subject)

    ENTITY_DECL_FIELD(std::string, City)
    ENTITY_DECL_FIELD(std::string, CommonName)
    ENTITY_DECL_FIELD(std::string, Country)
    ENTITY_DECL_FIELD(std::string, DisplayString)
    ENTITY_DECL_FIELD(std::string, Email)
    ENTITY_DECL_FIELD(std::string, Organization)
    ENTITY_DECL_FIELD(std::string, OrganizationalUnit)
    ENTITY_DECL_FIELD(std::string, State)
    ENTITY_DECL_FIELD(std::string, Algorithm)

    ENTITY_DECL_FIELD(std::string, ValidNotAfter)
    ENTITY_DECL_FIELD(std::string, ValidNotBefore)
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, AdditionalCommonNames, {})
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, DomainComponents, {})
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>,
                          AdditionalOrganizationalUnits, {})
    ENTITY_DECL_FIELD_DEF(std::vector<int>, KeyUsage, {})
    ENTITY_DECL_FIELD_ENUM(Purpose, Purpose, unknown)

    CertificateBase() : Collection()
    {
        this->createMember(fieldCity);
        this->createMember(fieldAdditionalCommonNames);
        this->createMember(fieldAdditionalOrganizationalUnits);
        this->createMember(fieldCommonName);
        this->createMember(fieldCountry);
        this->createMember(fieldEmail);
        this->createMember(fieldOrganization);
        this->createMember(fieldOrganizationalUnit);
        this->createMember(fieldState);
        this->createMember(fieldAlgorithm);

        this->createMember(fieldIssuerCity);
        this->createMember(fieldIssuerAdditionalCommonNames);
        this->createMember(fieldIssuerAdditionalOrganizationalUnits);
        this->createMember(fieldIssuerCommonName);
        this->createMember(fieldIssuerCountry);
        this->createMember(fieldIssuerEmail);
        this->createMember(fieldIssuerOrganization);
        this->createMember(fieldIssuerOrganizationalUnit);
        this->createMember(fieldIssuerState);
        this->createMember(fieldIssuerAlgorithm);
        this->createMember(fieldIssuerKeyUsage);
    }
    ~CertificateBase() override = default;

  protected:
    class Query : public dbus::FindObjectDBusQuery
    {
        class FormatKeyUsage : public query::dbus::IFormatter
        {
          public:
            ~FormatKeyUsage() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<std::string, KeyUsage> actions{
                    {"DigitalSignature", KeyUsage::digitalSignature},
                    {"NonRepudiation", KeyUsage::nonRepudiation},
                    {"KeyEncipherment", KeyUsage::keyEncipherment},
                    {"DataEncipherment", KeyUsage::dataEncipherment},
                    {"KeyAgreement", KeyUsage::keyAgreement},
                    {"KeyCertSign", KeyUsage::keyCertSign},
                    {"CRLSigning", KeyUsage::crlSigning},
                    {"EncipherOnly", KeyUsage::encipherOnly},
                    {"DecipherOnly", KeyUsage::decipherOnly},
                    {"ServerAuthentication", KeyUsage::serverAuthentication},
                    {"ClientAuthentication", KeyUsage::clientAuthentication},
                    {"CodeSigning", KeyUsage::codeSigning},
                    {"EmailProtection", KeyUsage::emailProtection},
                    {"Timestamping", KeyUsage::timestamping},
                    {"CSPSigning", KeyUsage::ocspSigning}};
                // clang-format: on
                return formatValueFromDict(actions, property, value,
                                           KeyUsage::clientAuthentication);
            }
        };

      protected:
        static constexpr const char* certificateIface =
            "xyz.openbmc_project.Certs.Certificate";

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                certificateIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldCertificateString),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldIssuer),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldSubject),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldKeyUsage),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldValidNotAfter,
                    DBUS_QUERY_EP_CSTR(FormatTime)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldValidNotBefore,
                    DBUS_QUERY_EP_CSTR(FormatTime)
                ),
            )
        )
      protected:
        /* clang-format on */
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            this->setId(instance);
            this->setPurpose(instance);
            this->setCertificateData(instance, getFieldIssuer(instance),
                                     fieldIssuer);
            this->setCertificateData(instance, getFieldSubject(instance));
        }
        void setId(const DBusInstancePtr& instance) const
        {
            const auto& objectPath = instance->getObjectPath();
            setFieldId(instance, getNameFromLastSegmentObjectPath(objectPath));
        }
        void setCertificateData(const DBusInstancePtr& instance,
                                const std::string& raw,
                                const std::string& fieldNamePrefix = "") const
        {
            static constexpr const char* dc = "DC";
            static constexpr const char* altName = "subjectAltName";
            static constexpr const char* usage = "extendedKeyUsage";

            static const std::map<std::string, std::string> dictionary{
                {"L", fieldCity},
                {"CN", fieldCommonName},
                {"C", fieldCountry},
                {"algorithm", fieldAlgorithm},
                {"emailAddress", fieldEmail},
                {"O", fieldOrganization},
                {"OU", fieldOrganizationalUnit},
                {"ST", fieldState},
                {dc, fieldDomainComponents},
                {altName, fieldAdditionalCommonNames},
                {usage, fieldKeyUsage},
            };
            std::map<std::string, std::vector<std::string>> listFieldValues{
                {dc, {}},
                {altName, {}},
                {usage, {}},
            };
            const auto entries = splitToVector(std::stringstream(raw), ',');
            for (const auto& entry : entries)
            {
                const auto [key, value] =
                    helpers::utils::splitToPair(entry, '=');
                auto fieldIt = dictionary.find(key);
                if (fieldIt != dictionary.end())
                {
                    const auto field = fieldNamePrefix + fieldIt->second;
                    auto listFieldIt = listFieldValues.find(key);

                    if (listFieldIt != listFieldValues.end())
                    {
                        listFieldIt->second.push_back(value);
                        continue;
                    }
                    instance->supplementOrUpdate(field, value);
                }
            }
            for (const auto& [key, dict] : listFieldValues)
            {
                const auto field = fieldNamePrefix + dictionary.at(key);
                if (key == usage)
                {
                    this->setKeyUasge(instance, dict, fieldNamePrefix);
                    continue;
                }
                instance->supplementOrUpdate(field, dict);
            }
        }
        void setKeyUasge(const DBusInstancePtr& instance,
                         const std::vector<std::string>& dictionary,
                         const std::string& fieldNamePrefix = "") const
        {
            FormatKeyUsage formatter;
            std::vector<int> values;
            const auto field = fieldNamePrefix + fieldKeyUsage;
            for (const auto& keyUsageStr : dictionary)
            {
                values.emplace_back(
                    std::get<int>(formatter.format(field, keyUsageStr)));
            }
            instance->supplementOrUpdate(field, values);
        }
        virtual void setPurpose(const DBusInstancePtr& instance) const = 0;
    };
};

class TrustStoreCertificates final :
    public CertificateBase,
    public NamedEntity<TrustStoreCertificates>
{
    using BaseQuery = CertificateBase::Query;
    static constexpr const char* trustStoreService =
        "xyz.openbmc_project.Certs.Manager.Authority.Ldap";
    class TrustStoreQuery : public BaseQuery
    {
      public:
        TrustStoreQuery() : BaseQuery()
        {}
        ~TrustStoreQuery() override = default;

        /* clang-format off */
      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/certs/authority/ldap/",
            DBUS_QUERY_CRIT_IFACES(BaseQuery::certificateIface),
            noDepth,
            trustStoreService
        )
        /* clang-format on */
        void setPurpose(const DBusInstancePtr& instance) const override
        {
            CertificateBase::setFieldPurpose(
                instance, CertificateBase::Purpose::trustStore);
        }
    };

  public:
    TrustStoreCertificates() :
        CertificateBase(), query(std::make_shared<TrustStoreQuery>())
    {}
    ~TrustStoreCertificates() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class LDAPCertificates final :
    public CertificateBase,
    public NamedEntity<LDAPCertificates>
{
    using BaseQuery = CertificateBase::Query;
    static constexpr const char* ldapCertsService =
        "xyz.openbmc_project.Certs.Manager.Client.Ldap";
    class LDAPQuery : public BaseQuery
    {
      public:
        LDAPQuery() : BaseQuery()
        {}
        ~LDAPQuery() override = default;

        /* clang-format off */
      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/certs/client/ldap/",
            DBUS_QUERY_CRIT_IFACES(BaseQuery::certificateIface),
            noDepth,
            ldapCertsService
        )
        /* clang-format on */
        void setPurpose(const DBusInstancePtr& instance) const override
        {
            CertificateBase::setFieldPurpose(instance,
                                             CertificateBase::Purpose::ldap);
        }
    };

  public:
    LDAPCertificates() : CertificateBase(), query(std::make_shared<LDAPQuery>())
    {}
    ~LDAPCertificates() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class HTTPSCertificates final :
    public CertificateBase,
    public NamedEntity<HTTPSCertificates>
{
    using BaseQuery = CertificateBase::Query;
    static constexpr const char* httpsCertsService =
        "xyz.openbmc_project.Certs.Manager.Server.Https";
    class HTTPSQuery : public BaseQuery
    {
      public:
        HTTPSQuery() : BaseQuery()
        {}
        ~HTTPSQuery() override = default;

        /* clang-format off */
      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/certs/server/https/",
            DBUS_QUERY_CRIT_IFACES(BaseQuery::certificateIface),
            noDepth,
            httpsCertsService
        )
        /* clang-format on */
        void setPurpose(const DBusInstancePtr& instance) const override
        {
            CertificateBase::setFieldPurpose(instance,
                                             CertificateBase::Purpose::https);
        }
    };

  public:
    HTTPSCertificates() :
        CertificateBase(), query(std::make_shared<HTTPSQuery>())
    {}
    ~HTTPSCertificates() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
