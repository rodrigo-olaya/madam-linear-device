# MDADM Linear Device on Simulated JBOD

## Overview

This project implements an MDADM Linear device in C and Linux, running on an existing simulated JBOD (Just a Bunch of Disks). The project includes functionality for reading and writing data across multiple disks, caching for performance improvements, and network communication to enable remote access.

## What is an MDADM Linear Device?

MDADM (Multiple Device Administration) is a Linux utility used to manage software RAID arrays. A Linear RAID array (RAID 0 without striping) concatenates multiple disks into a single logical volume, where data is written sequentially from one disk to the next. Unlike other RAID levels, it does not provide redundancy but allows for the seamless expansion of storage capacity.

## What is JBOD (Just a Bunch of Disks)?

JBOD is a storage architecture where multiple disks are combined without any RAID configuration. Each disk operates independently, and data is stored across disks as needed. Unlike RAID, JBOD does not provide redundancy or striping but allows for flexible storage expansion.

## Purpose of This Project

The goal of this project is to simulate a linear RAID (RAID 0 without striping) storage system using MDADM principles on a JBOD (Just a Bunch of Disks) simulated environment. This implementation creates a single logical storage volume over multiple independent disks, allowing seamless data storage across them.

## Project Components

The project consists of the following key parts:

### 1. Read Implementation

Uses jbod_operation() to read data from the JBOD disks.

Reads data into a buffer using an address and length.

Requires precise memory management.

### 2. Write Implementation

Uses jbod_operation() to write data to the JBOD disks.

Implements permission control via mdadm_write_permission() and mdadm_revoke_write_permission().

Trace files verify memory integrity across operations.

### 3. Cache Implementation

Enhances read/write performance by caching frequently accessed data.

Improves efficiency by reducing redundant disk operations.

### 4. Network Implementation

Implements a client-server model for remote JBOD access.

Sends and receives operation packets over a network.

Handles socket connections and error recovery.
