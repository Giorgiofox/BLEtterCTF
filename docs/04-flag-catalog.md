# Flag catalog

The full BLEtterCTF challenge set. Each flag teaches one BLE primitive. This is
player-facing: it lists the goal and the primitive, not the answer.

Legend for "Solve with": **H** = any Mac/PC/phone, **S** = ButteRFly sniffer,
**P** = nRF52840 probe.

## T0 - GATT base
| # | Challenge | Primitive | Solve with |
|---|-----------|-----------|:---------:|
| 1 | Read the gift flag | connect + read + submit loop | H |
| 2 | Flag in the value | characteristic read | H |
| 3 | Flag in a descriptor | User Description (0x2901) | H |
| 4 | Flag in the device name | GAP standard characteristics | H |
| 5 | Decode the value | hex encoding | H |
| 6 | Decode the value again | ASCII / base64 | H |
| 7 | Collect the pieces | enumerate every characteristic | H |
| 8 | Write to unlock | write then read | H |
| 9 | Unlock the sequence | state machine over GATT | H |

## T1 - Events
| # | Challenge | Primitive | Solve with |
|---|-----------|-----------|:---------:|
| 10 | Subscribe for the flag | CCCD / notify | H |
| 11 | Acknowledged delivery | indicate vs notify | H |
| 12 | Reassemble the stream | multi-packet notify | H |
| 13 | Trigger then listen | notify + write interplay | H |
| 14 | Property vs reality | hidden notify (write CCCD anyway) | H |

## T2 - Advertising
| # | Challenge | Primitive | Solve with |
|---|-----------|-----------|:---------:|
| 15 | Flag in the beacon | manufacturer-specific data | H |
| 16 | Ask for more | scan response (active scan) | H |
| 17 | Typed data | service data AD type | H |
| 18 | Catch it in time | rotating adv value | H |
| 19 | Beyond legacy | extended advertising (BLE5) | S / P |
| 20 | Sync up | periodic advertising | S / P |

## T3 - Security / SMP
| # | Challenge | Primitive | Solve with |
|---|-----------|-----------|:---------:|
| 21 | Encrypted read | encryption required -> Just Works pairing | H |
| 22 | Prove it is you | Passkey entry (MITM protection) | H |
| 23 | Confirm the number | LESC Numeric Comparison | H |
| 24 | Come back later | bonding persistence | H |
| 25 | Out of band | OOB pairing (key via serial / BLEtterCap) | P |
| 26 | Why LESC matters | sniff legacy plaintext vs LESC-protected | S |

## T4 - Privacy
| # | Challenge | Primitive | Solve with |
|---|-----------|-----------|:---------:|
| 27 | Same device, new address | RPA + IRK resolution | S |
| 28 | For your eyes only | directed advertising | P |
| 29 | Show your identity | address filtering (redesigned classic #15) | P |

## T5 - PHY & BLE 5.x
| # | Challenge | Primitive | Solve with |
|---|-----------|-----------|:---------:|
| 30 | Double speed | 2M PHY | P |
| 31 | Long range | LE Coded PHY (S8) | P / S |
| 32 | Read the timing | connection parameter update (observe) | S |

## T6 - Advanced
| # | Challenge | Primitive | Solve with |
|---|-----------|-----------|:---------:|
| 33 | Open a channel | L2CAP CoC (credit-based) | H / P |
| 34 | The database moved | GATT service changed / robust caching | H |
| 35 | Break it gently | safe fuzzing (malformed / edge write) | H |
| 36 | Too big to fit | long write (prepare / execute) | H |
| 37 | Capstone | chain sniff + pair + notify | all |
| 38 | Look outside | OSINT / recon | - |

## Implementation status

The scaffold firmware currently implements flags **1, 3, 8, 10, 15** as one
working example per mechanic class. The rest are ported in per
`docs/07-adding-flags.md`.
