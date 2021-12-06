import { EventEmitter } from 'events';

export interface SOptions {
  bufferSize?: number;
  addressFamily?: number;
  protocol?: number;
}

export const SocketLevel: {
  IPPROTO_IP: 0;
  IPPROTO_IPV6: 41;
  SOL_SOCKET: 65535;
};

export const SocketOption: {
  IPV6_TTL: 4;
  IPV6_UNICAST_HOPS: 4;
  IPV6_V6ONLY: 27;
  IP_HDRINCL: 2;
  IP_OPTIONS: 1;
  IP_TOS: 3;
  IP_TTL: 4;
  SO_BROADCAST: 32;
  SO_RCVBUF: 4098;
  SO_RCVTIMEO: 4102;
  SO_SNDBUF: 4097;
  SO_SNDTIMEO: 4101;
};

// The internal C++ socket:
export class SocketWrap {
  htonl(n: number): number;
  htons(n: number): number;
  ntohl(n: number): number;
  ntohs(n: number): number;

  // Also contains SocketLevel/SocketOption, but
  // skipped.
}

export declare interface Socket {
  on(event: 'close'): this;
  on(event: 'error', listener: (error: string) => void): this;
  on(
    event: 'message',
    listener: (message: Buffer, address: string) => void,
  ): this;
}

export class Socket extends EventEmitter {
  constructor(options?: SOptions);

  wrap: SocketWrap;
  requests: any[];
  buffer: Buffer;

  recvPaused: boolean;
  sendPaused: boolean;

  close(): this;

  getOption(
    level: number,
    option: number,
    value: Buffer,
    length: number,
  ): number;

  onClose(): void;

  onError(error: any): void;

  onRecvReady(): Socket;

  onSendReady(): void;

  pauseRecv(): Socket;

  pauseSend(): Socket;

  resumeRecv(): Socket;

  resumeSend(): Socket;

  send(
    buffer: Buffer,
    offset: number,
    length: number,
    address: any,
    // eslint-disable-next-line @typescript-eslint/ban-types
    beforeCallback: Function,
    // eslint-disable-next-line @typescript-eslint/ban-types
    afterCallback?: Function,
  ): Socket;

  setOption(
    level: number,
    option: number,
    value: Buffer,
    length?: number,
  ): void;
}

export const AddressFamily: {
  IPv4: 1;
  IPv6: 2;
};

export const Protocol: {
  None: 0;
  ICMP: 1;
  TCP: 6;
  UDP: 17;
  ICMPv6: 58;
};

// Looks like this can take some object with a .buffer prop/method as well
interface BufferableObject {
  buffer?: Buffer;
}

export function createChecksum(...args: Buffer[] | BufferableObject[]): number;

export function createSocket(options?: SOptions): Socket;

export function htonl(n: number): number;
export function htons(n: number): number;
export function ntohl(n: number): number;
export function ntohs(n: number): number;

export function writeChecksum(
  buffer: Buffer,
  offset: number,
  checksum: any,
): Buffer;
