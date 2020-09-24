## Concurrent EVM-based Blockchain

This repository contains a concurrent EVM-based blockchain, which contains a concurrent transaction execution engine to dynamically identify atomic section for each transaction and parallel execute transactions under the protection of HTM.

This concurrent blockchain is implemented based on FISCO BCOS which is implemented based on  c++ Ethereum.


## Quick Start

Read [Quick Start](https://fisco-bcos-documentation.readthedocs.io/en/latest/docs/installation.html) to learn the installation and deployment procedures

## PreCompiled Contract for concurrency 

Our group developed two precompiled contracts whose address is `0x1006` and `0x1007`, the contract in`0x1006` dynamically decided the atomic section for each transaction via identify the interface `begin_atomic()` and `end_atomic()`.  Another contract in `0x1007` is used to record the conflict relationship for functions of a contract into a KVTable.

## Concurrency Control

For miner, the execution of atomic sections  in transaction are executed under the protection of HTM, specifically, our group implemented the HTM using the interface from Intel tbb ( `tbb::speculative_spin_mutex`). The mutex is implemented by using the Hardware Lock Elision (HLE) which is  an instruction prefix-based interface  in Intel TSX. 

For validator, the concurrency control is based on happen-before graph which ensure that conflict atomic sections will never be executed concurrently.  

## Low-Conflict  Transaction Selector

This module can select transactions with a low conflict rate for concurrent execution for both miner and validator.For miner, we create an undirected graph for transactions, and based on the connectivity relationship of the graph, select the transaction set with the least conflict. For validator, we create a happen-before graph, and select transaction set with lowest in-degree.