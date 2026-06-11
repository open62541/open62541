# Security Policy

## Supported Versions

The open62541 releases are organized into release families. The following table
shows the current nomenclature concerning development and stable release
families.

| Release Family | Status              | Branch      |
| -------------- | ------------------- | ----------- |
| -              | development         | master      |
| v1.5.x         | stable              | 1.5         |
| v1.4.x         | oldstable           | 1.4         |
| v1.3.x         | oldoldstable        | 1.3         |

Beyond the public Github, you can get professional support for open62541 from
*o6 Automation GmbH* (https://www.o6-automation.com/services). This includes our
**Vulnerability Management Process** and **Long-Term Support** for older stable
release families. So software based on open62541 can be deployed and supported
in the field for many years.

## Reporting a Vulnerability

You are invited to disclose your findings privately with one of the following
mechanisms. **DO NOT OPEN PUBLIC GITHUB ISSUES FOR POTENTIAL VULNERABILITIES.**
This only gives the bad guys a head-start and does not speed up the
vulnerability handling.

- Via email to the mailing list open62541-security@googlegroups.com
- Via the Github disclosure mechanism at https://github.com/open62541/open62541/security/advisories

You can encrypt emails to us with this PGP key:

```
-----BEGIN PGP PUBLIC KEY BLOCK-----

mDMEZyvNHBYJKwYBBAHaRw8BAQdAVVciLHk9qEu38ZmqGfUuB9SD7lvw6Z8lTm6G
H2zqh4O0NG9wZW42MjU0MSBUZWFtIDxvcGVuNjI1NDEtc2VjdXJpdHlAZ29vZ2xl
Z3JvdXBzLmNvbT6ImQQTFgoAQRYhBMlp8zR7pjG9VoaVFK5VKNbXA7F8BQJnK80c
AhsDBQkFoxmUBQsJCAcCAiICBhUKCQgLAgQWAgMBAh4HAheAAAoJEK5VKNbXA7F8
vLcBAIC7/R5gZrqXm+js+tQrMgua/7Rr8h2CGC8GVogwLmYBAQDF9XzoZMBPQu5j
Vtudpc3lzQy4g8qzIvtwTaQe4KOhCLg4BGcrzRwSCisGAQQBl1UBBQEBB0Acmd51
rRZ3697if50xOUeu2tdHjOWMn+P3Ga5/2ZIGKwMBCAeIfgQYFgoAJhYhBMlp8zR7
pjG9VoaVFK5VKNbXA7F8BQJnK80cAhsMBQkFoxmUAAoJEK5VKNbXA7F8y4UA/RSe
NKKvTqtDayyNn6kRKLnuBAPlXTjvpMARcSMFe9APAQCdu22yS4KB3cGBHoXMSTwO
tfp1v8HATMXKB65FmujmBg==
=Juz6
-----END PGP PUBLIC KEY BLOCK-----
```

## Vulnerability Management Process

The disclosure of a potential vulnerability triggers our Vulnerability
Management Process. It comprises of the following steps:

1. Evaluation of the disclosure (CVSS score)
2. If relevant, preparation of mitigations (patches) for the impacted open62541
   release families together with a non-public Vulnerability Advisory
3. Dissemination to commercial users and operators of critical infrastructure
4. Embargo time (typically 30 days)
5. Merge of the mitigations into the impacted open62541 release family branches
6. Preparation of open62541 patch releases for the impacted release families

## Acknowledgement and Public CVE

**We do not endorse public CVE advisories for open62541.** With todays AI-based
coding tools, a public CVE has roughly the same impact as releasing a working
exploit. Professional users and operators of critical installations receive
non-public Vulnerability Advisories via the Professional Support Services. Then,
after an embargo time to fix critical installations, we prepare public patch
releases for the impacted versions of open62541.

If the person disclosing a vulnerability wishes so, we can give a personal
acknowledgement of the disclosure both in the non-public Vulnerability Advisory
and in the commits that are eventually merged into the public git branches.
