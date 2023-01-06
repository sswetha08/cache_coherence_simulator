## Cache Coherence Simulator

### Objective
To study and compare the performance of different kinds of bus-based coherence protocols.

This project simulates cache coherence for 3 different protocols: 
* MSI
* MSI (With BusUpgr signal)
* MESI 

Metrics like number of interventions, invalidations and snoops are collected to evaluate performance.
Additionally, a history filter has been implemented to reduce the number of wasteful snoops on the receiving core side and improve performance.
