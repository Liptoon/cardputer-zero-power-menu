#!/usr/bin/env bash
# Deploys the power menu to a CardputerZero over SSH.
#
# The target must be reachable with a key (BatchMode). Root is required to
# write into /usr/share/APPLaunch; when the login user is not root the script
# stops and prints the exact commands to finish manually.
#
# Usage:
#   tools/deploy.sh --host pi@cardputer.local
#   CARDPUTER_HOST=pi@cardputer.local tools/deploy.sh --no-build --method copy
#
# Options:
#   -H, --host USER@HOST   Target (or CARDPUTER_HOST)
#   -p, --port PORT        SSH port (or CARDPUTER_PORT, default 22)
#   -i, --key FILE         SSH identity (or CARDPUTER_KEY)
#       --no-build         Skip 'make device && make deb'
#       --method METHOD    deb (default) or copy (tar over ssh)
#       --dry-run          Print the remote commands without running them
#   -h, --help             This help

set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_root"

app_slug="power-menu"
app_version="${APP_VERSION:-0.1.2}"
deb_path="dist/${app_slug}_${app_version}_arm64.deb"
stage_tree="build/stage/usr/share/APPLaunch"

host="${CARDPUTER_HOST:-}"
port="${CARDPUTER_PORT:-22}"
identity="${CARDPUTER_KEY:-}"
remote_dir="${CARDPUTER_TMP:-/tmp}"
do_build=1
method="deb"
dry_run=0

usage() {
    sed -n '2,25p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

while [ $# -gt 0 ]; do
    case "$1" in
        -H|--host) host="${2:-}"; shift 2 ;;
        -p|--port) port="${2:-}"; shift 2 ;;
        -i|--key) identity="${2:-}"; shift 2 ;;
        --no-build) do_build=0; shift ;;
        --method) method="${2:-}"; shift 2 ;;
        --dry-run) dry_run=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "error: unknown argument '$1'" >&2; usage; exit 2 ;;
    esac
done

if [ -z "$host" ]; then
    echo "error: no target host. Use --host user@host or CARDPUTER_HOST=user@host" >&2
    exit 2
fi

case "$method" in
    deb|copy) ;;
    *) echo "error: --method must be 'deb' or 'copy'" >&2; exit 2 ;;
esac

ssh_opts=(-o BatchMode=yes -o ConnectTimeout=10 -o StrictHostKeyChecking=accept-new)
if [ "$port" != "22" ]; then
    ssh_opts+=(-p "$port")
fi
if [ -n "$identity" ]; then
    ssh_opts+=(-i "$identity")
fi

remote() {
    if [ "$dry_run" -eq 1 ]; then
        echo "[dry-run] ssh $host $*"
    else
        ssh "${ssh_opts[@]}" "$host" "$@"
    fi
}

send_file() {
    if [ "$dry_run" -eq 1 ]; then
        printf '[dry-run] scp %s %s:%s/\n' "$1" "$host" "$remote_dir"
    else
        scp "${ssh_opts[@]}" "$1" "$host:$remote_dir/"
    fi
}

if [ "$do_build" -eq 1 ] && [ "$dry_run" -eq 0 ]; then
    echo "==> building AArch64 package"
    make device
    make deb
fi

if [ "$method" = "deb" ] && [ "$dry_run" -eq 0 ] && [ ! -f "$deb_path" ]; then
    echo "error: $deb_path not found; run 'make device && make deb' first" >&2
    exit 1
fi

echo "==> checking target $host"
if [ "$dry_run" -eq 0 ]; then
    arch="$(remote 'uname -m')"
    case "$arch" in
        aarch64|arm64) ;;
        *) echo "error: target reports '$arch', expected aarch64" >&2; exit 1 ;;
    esac

    remote 'test -d /usr/share/APPLaunch' || {
        echo "error: /usr/share/APPLaunch missing on target (APPLaunch not installed?)" >&2
        exit 1
    }

    remote_uid="$(ssh "${ssh_opts[@]}" "$host" 'id -u')"
    if [ "$remote_uid" != "0" ]; then
        echo "error: '$host' logs in as uid $remote_uid, root is required to install." >&2
        echo "finish manually with the packaging files under build/stage:" >&2
        if [ "$method" = "deb" ]; then
            echo "  scp ${deb_path} ${host}:${remote_dir}/" >&2
            echo "  ssh ${host} 'sudo dpkg -i ${remote_dir}/$(basename "$deb_path")'" >&2
            echo "  # no restart needed: APPLaunch detects new entries by itself" >&2
        else
            echo "  tar -C ${stage_tree} -czf - . | ssh ${host} 'sudo tar -C /usr/share/APPLaunch -xzf -'" >&2
        fi
        exit 3
    fi
fi

if [ "$method" = "deb" ]; then
    echo "==> uploading $(basename "$deb_path")"
    send_file "$deb_path"
    echo "==> installing package"
    remote "dpkg -i '${remote_dir}/$(basename "$deb_path")'"
else
    [ -d "$stage_tree" ] || { echo "error: $stage_tree missing; run 'make stage' first" >&2; exit 1; }
    echo "==> copying APPLaunch tree"
    if [ "$dry_run" -eq 1 ]; then
        echo "[dry-run] tar -C $stage_tree -czf - . | ssh $host 'tar -C /usr/share/APPLaunch -xzf -'"
    else
        tar -C "$stage_tree" -czf - . | ssh "${ssh_opts[@]}" "$host" 'tar -C /usr/share/APPLaunch -xzf -'
    fi
fi

echo "==> restarting APPLaunch (when a systemd unit exists)"
remote 'systemctl restart APPLaunch.service 2>/dev/null || systemctl restart applaunch.service 2>/dev/null || echo "no APPLaunch systemd unit; launcher will pick up the entry automatically"'

echo "==> verifying"
remote "test -f /usr/share/APPLaunch/applications/${app_slug}.desktop && \
        test -x /usr/share/APPLaunch/bin/${app_slug} && \
        test -f /usr/share/APPLaunch/share/images/${app_slug}.png && \
        echo 'entry, binary and icon present'"

echo "done: Power should now appear in the APPLaunch carousel"
