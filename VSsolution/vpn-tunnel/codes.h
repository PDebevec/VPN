#pragma once

constexpr byte INIT_STATE = 0x0;

/// <summary>
/// BaseSocket class codes
/// </summary>
constexpr byte SOCKET_STOPPED		= 0x10;
constexpr byte SOCKET_INITIALIZED	= 0x11;

/// <summary>
/// BaseWindivert class codes
/// </summary>
constexpr byte WD_CLOSED		= 0x20;
constexpr byte WD_OPENED		= 0x21;

/// <summary>
/// TCPSocket class codes
/// </summary>
constexpr byte TCP_STOPPED		= 0x30;
constexpr byte TCP_INITIALIZED	= 0x31;
constexpr byte TCP_LISTENING	= 0x32;
constexpr byte TCP_CONNECTED	= 0x33;

/// <summary>
/// BaseUDP class codes
/// </summary>
constexpr byte UDP_STOPPED		= 0x40;
constexpr byte UDP_INITIALIZED	= 0x41;

/// <summary>
/// Tunnel class codes
/// </summary>
constexpr byte TUNNEL_STOP			= 0x50;
constexpr byte TUNNEL_INIT			= 0x51;
constexpr byte TUNNEL_INITIALIZED	= 0x52;
constexpr byte TUNNEL_CONNECT		= 0x53;
constexpr byte TUNNEL_CONNECTED		= 0x54;
constexpr byte TUNNEL_START			= 0x55;
constexpr byte TUNNEL_STARTED		= 0x56;
constexpr byte TUNNEL_LOOP			= 0x57;
constexpr byte TUNNEL_DESTORY		= 0x58;
constexpr byte TUNNEL_ERROR			= 0x59;

/// <summary>
/// Tunnel class codes
/// </summary>
constexpr byte VPN_STOP		= 0x60;
constexpr byte VPN_INIT		= 0x61;
constexpr byte VPN_STARED	= 0x62;
constexpr byte VPN_DESTORY	= 0x63;
constexpr byte VPN_ERROR	= 0x64;


