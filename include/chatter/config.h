#ifndef _CH_CONFIG_H_
#define _CH_CONFIG_H_

#include <climits>

namespace chatter {

const int kMaxUDPPayloadSize = 65507;

/* Timeout for connection attempts (connecting peers) */
const int kConnectTimeOut = 5 * 1000; /* 5 seconds */

/* Timeout for connected peers */
const int kPeerTimeOut = 20 * 1000; /* 20 seconds */

/* If no data received after this period of time, send a ping on this interval */
const int kPingInterval = 1 * 1000;

/* Back off factor - retransmission interval is multiplied by this for each retransmit */
const int kRetransmissionBackOffFactor = 2;

/* Retransmission interval limits (back off will not cause interval to exceed this) */
const int kMinRetransmissionInterval = 10;
const int kMaxRetransmissionInterval = 3 * 1000; /* 3 seconds */

const int kMTU = 1500; /* TODO(ben): more specific value needed. */

/* Congestion control */
const int kCongestionInc = kMTU;
const int kCongestionDecFactor = 2;
const int kMinCongestionWindow = kMTU * 2;
const int kMaxCongestionWindow = INT_MAX - kCongestionInc - 1;

} // namespace chatter

#endif // _CH_CONFIG_H_
