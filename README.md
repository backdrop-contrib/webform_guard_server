# Webform Guard Server
**Webform Guard Server** — A centralised REST API service for blocking spam webform submissions across multiple Backdrop CMS sites.

Webform Guard Server is a Backdrop CMS module that acts as the intelligence hub for the Webform Guard system. It receives spam-check requests from remote client sites running the companion `webform_guard_client` module, validates API credentials, checks submissions against a shared blocklist, and returns a verdict. A single Webform Guard Server installation can protect an unlimited number of client sites, each with its own API key and subscription tier. Run your own server without restrictions, even on the same Backdrop CMS installation as the client module, or use a hosted service — see the Credits section for details.

## Beta Release Notes
As a beta release, the core API, blocklist engine, client registry, and admin interface are fully functional. The module is stable enough for testing in development environments. Before deploying to production, ensure your Backdrop CMS environment is running PHP 8.0+ and clear your system caches after installation to register the API routes and admin menu items.

## Features

* **Centralised Spam Blocklist:** A single shared blocklist protects all registered client sites simultaneously. Spam reported by any client is blocked for all.
* **Per-Client API Keys:** Each client site gets its own unique API key and subscription record. Compromise one key without affecting others.
* **Subscription Management:** Three-tier subscription system (Free, Active, Expired) with optional expiry dates. Expired subscriptions degrade gracefully — submissions are allowed through rather than blocked.
* **Three REST API Endpoints:** `POST /api/v1/webform-guard/check-submission` to validate a submission, `POST /api/v1/webform-guard/report-spam` to add an identifier to the blocklist, and `GET /api/v1/webform-guard/status` for health checks and connection testing. The `api/v1/webform-guard/` namespace ensures these routes do not conflict with other API modules installed on the same server.
* **Token-Based Spam Reporting:** A public `report/{token}` page allows recipients to report spam directly from a link in their notification email — no login required. Tokens are HMAC-SHA256 signed with the client's API key and expire after 30 days.
* **Admin UI:** Full admin interface for managing registered clients (add, edit, delete, generate API keys), viewing and removing blocklist entries, and toggling spam reporting on or off.
* **Views Integration:** Exposes the spam identifier blocklist and per-site blocked-submission metrics as Views base tables for custom reporting.
* **Backdrop Native:** Built exclusively for Backdrop CMS using strict PHP 8.0+ standards and Backdrop APIs throughout.

## Requirements

- Backdrop CMS 1.x
- PHP 8.0+ (While the code may technically function with PHP 7.4 at this time, we strictly require PHP 8.0+ and will not address issues related to older PHP versions.)
- Companion module: [Webform Guard Client](../webform_guard_client) (installed on each client site)

## Installation

Install this module using the official Backdrop CMS instructions at https://docs.backdropcms.org/documentation/extend-with-modules

Enable the module on the site that will act as your guard server.

## Configuration

1. Navigate to **Admin → Configuration → Webform Guard Server → Clients** and add a record for each client site you wish to protect.
2. For each client, click **Generate API key** and copy the generated key — you will paste this into the corresponding `webform_guard_client` settings on the client site.
3. Set the **Site identifier** to match what the client module will send (recommended: the client site's domain, e.g. `mysite.co.uk`).
4. Set the **Subscription status** to Free or Active as appropriate.
5. Under the **Settings** tab, enable or disable the spam reporting endpoint (`POST /api/v1/report-spam`) as required.

Client sites should point their **Guard server base URL** to this installation (e.g. `https://guard.example.com`) — the client module appends the API paths automatically.

## Issues

Bugs and feature requests should be reported in the Issue Queue: https://github.com/albanycomputers/webform_guard/issues

## Current Maintainer(s)
- Steve Moorhouse (albanycomputers) (https://github.com/albanycomputers)
- Additional maintainers and contributors welcome.

## Planned Features

The following are on the roadmap but not yet implemented:

* **Additional identifier field types:** Name (HTML already stripped), telephone, and website/URL fields can already be sent as spam identifiers and are normalised correctly. Future releases will document these officially and may add further field-type-specific rules.
* **Country-specific phone normalisation:** Optionally normalise local phone formats (e.g. UK `07xxx` → `+447xxx`) for cross-format matching. Requires a per-server locale setting.
* **URL normalisation improvements:** Strip `www.` prefix and normalise `http`/`https` variants so they resolve to the same blocklist entry.
* **Rate limiting:** Per-client request limits on the check-submission endpoint.
* **Bulk blocklist management:** Import and export the blocklist as CSV or JSON via the admin UI.
* **Per-client blocklists:** Allow individual client sites to maintain their own private blocklist in addition to the shared global one.
* **Alter hook for normalisation:** A `backdrop_alter()` hook to let other modules register custom field-type normalisation rules without patching this module.
* **Word and phrase blocklist:** A server-admin-managed list of blocked words and phrases checked against all submitted field values. No client changes required — the full submission payload is already sent. Planned support for contains, exact, and regex match types.
* **Spam submission storage and analysis:** When a submission is reported as spam, save the full field payload for admin review. Allows patterns — recurring phrases, links, or content — to be identified and added to the word blocklist with a single click, across all client sites simultaneously. Particularly useful as spammers cycle through email addresses but reuse the same message content.

## Credits
- Steve Moorhouse — Zulip (DrAlbany)
- Claude Code by Anthropic assisted with development of this module.

- Current development is sponsored by [Albany Computer Services](https://www.albany-computers.co.uk), providers of computer support, [web design](https://www.albanywebdesign.co.uk), and [web hosting](https://www.albany-hosting.co.uk).
- A hosted Webform Guard server is available for sites that prefer not to self-host. Contact the [maintainer](https://github.com/albanycomputers) to enquire.

## License
This project is GPL v2 or later software. See the LICENSE.txt file in this directory for complete text.
