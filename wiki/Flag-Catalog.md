# Flag Catalog

The full BLEtterCTF challenge set. Each flag teaches one BLE primitive. This is
player-facing: it lists the goal and the primitive, not the answer. For the
method, see the tier walkthroughs.

**Solve with:** **H** = any Mac/PC/phone, **S** = ButteRFly sniffer,
**P** = nRF52840 probe. **Status:** shows what the current firmware implements.

## T0 - GATT base
| # | Challenge | Primitive | Solve with | Status |
|---|-----------|-----------|:---------:|:------:|
| 1 | the gift | connect + read + submit loop | H | done |
| 2 | hello, world | characteristic read | H | done |
| 3 | read the fine print | User Description (0x2901) | H | done |
| 4 | flag in the device name | GAP standard characteristics | H | planned |
| 5 | hex marks the spot | hex encoding | H | done |
| 6 | sixty-four shades | base64 | H | done |
| 7 | collect all three | enumerate every characteristic | H | done |
| 8 | knock knock | write then read | H | done |
| 9 | the magic words, in order | state machine over GATT | H | done |

## T1 - Events
| # | Challenge | Primitive | Solve with | Status |
|---|-----------|-----------|:---------:|:------:|
| 10 | ring ring | CCCD / notify | H | done |
| 11 | read receipts on | indicate vs notify | H | done |
| 12 | the flag that would not fit | multi-packet notify | H | done |
| 13 | trigger then listen | notify + write interplay | H | planned |
| 14 | property vs reality | hidden notify | H | planned |

## T2 - Advertising
| # | Challenge | Primitive | Solve with | Status |
|---|-----------|-----------|:---------:|:------:|
| 15 | hidden in plain sight | manufacturer-specific data | H | done |
| 16 | ask nicely | scan response (active scan) | H | done |
| 17 | typed data | service data AD type | H | planned |
| 18 | catch it in time | rotating adv value | H | planned |
| 19 | beyond legacy | extended advertising (BLE5) | S / P | planned |
| 20 | sync up | periodic advertising | S / P | planned |

## T3 - Security / SMP
| # | Challenge | Primitive | Solve with | Status |
|---|-----------|-----------|:---------:|:------:|
| 21 | members only | encryption required, Just Works pairing | H | done |
| 22 | prove it is you | Passkey entry (MITM) | H | planned |
| 23 | confirm the number | LESC Numeric Comparison | H | planned |
| 24 | come back later | bonding persistence | H | planned |
| 25 | out of band | OOB pairing | P | planned |
| 26 | why LESC matters | sniff legacy vs LESC | S | planned |

## T4 - Privacy
| # | Challenge | Primitive | Solve with | Status |
|---|-----------|-----------|:---------:|:------:|
| 27 | same device, new address | RPA + IRK resolution | S | planned |
| 28 | for your eyes only | directed advertising | P | planned |
| 29 | show your identity | address filtering | P | planned |

## T5 - PHY & BLE 5.x
| # | Challenge | Primitive | Solve with | Status |
|---|-----------|-----------|:---------:|:------:|
| 30 | double speed | 2M PHY | P | planned |
| 31 | long range | LE Coded PHY (S8) | P / S | planned |
| 32 | read the timing | connection parameter update (observe) | S | planned |

## T6 - Advanced
| # | Challenge | Primitive | Solve with | Status |
|---|-----------|-----------|:---------:|:------:|
| 33 | open a channel | L2CAP CoC (credit-based) | H / P | planned |
| 34 | the database moved | GATT service changed / caching | H | planned |
| 35 | break it gently | safe fuzzing | H | planned |
| 36 | too big for one bite | long read (Read Blob) | H | done |
| 37 | capstone | chain sniff + pair + notify | all | planned |
| 38 | look outside | OSINT / recon | - | planned |

"planned" flags are finalized as the firmware grows and get tested on real
hardware. "done" flags are live in the current build.
