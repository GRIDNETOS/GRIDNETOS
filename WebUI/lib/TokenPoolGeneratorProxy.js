/**
 * @file TokenPoolGeneratorProxy.js
 * @brief Main thread proxy for Token Pool generation Web Worker
 * @version 1.0
 * @date 2025
 *
 * Provides async interface to offload token pool generation to Web Worker,
 * preventing main thread blocking during expensive hash chain computation.
 * Supports progress callbacks for UI feedback.
 */

'use strict';

import { CTokenPool, CTokenPoolBank } from './StateLessChannels.js';

/**
 * @class TokenPoolGeneratorProxy
 * @brief Singleton proxy for communicating with token pool generator worker
 */
export class TokenPoolGeneratorProxy {
    constructor() {
        if (TokenPoolGeneratorProxy._instance) {
            return TokenPoolGeneratorProxy._instance;
        }

        this.worker = null;
        this.isReady = false;
        this.pendingRequests = new Map();
        this.progressCallbacks = new Map();
        this.requestIdCounter = 0;
        this.readyPromise = null;

        this._initializeWorker();

        TokenPoolGeneratorProxy._instance = this;
    }

    /**
     * Get singleton instance
     */
    static getInstance() {
        if (!TokenPoolGeneratorProxy._instance) {
            TokenPoolGeneratorProxy._instance = new TokenPoolGeneratorProxy();
        }
        return TokenPoolGeneratorProxy._instance;
    }

    /**
     * Initialize the Web Worker
     */
    _initializeWorker() {
        try {
            this.worker = new Worker('/lib/TokenPoolGeneratorWorker.js');

            this.readyPromise = new Promise((resolve, reject) => {
                const timeout = setTimeout(() => {
                    reject(new Error('Token pool generator worker initialization timeout'));
                }, 5000);

                const initialHandler = (event) => {
                    if (event.data.type === 'ready') {
                        clearTimeout(timeout);
                        this.isReady = true;
                        resolve();
                        // Switch to normal message handler
                        this.worker.onmessage = this._handleWorkerMessage.bind(this);
                    }
                };

                this.worker.onmessage = initialHandler;

                this.worker.onerror = (error) => {
                    clearTimeout(timeout);
                    console.error('[TokenPoolGeneratorProxy] Worker error:', error);
                    reject(error);
                };
            });

        } catch (error) {
            console.error('[TokenPoolGeneratorProxy] Failed to initialize worker:', error);
            throw error;
        }
    }

    /**
     * Handle messages from worker
     */
    _handleWorkerMessage(event) {
        const response = event.data;

        if (response.type === 'progress') {
            // Handle progress update
            const callback = this.progressCallbacks.get(response.requestId);
            if (callback) {
                callback(response.data);
            }
            return;
        }

        if (response.type === 'result') {
            const { id, success, data, error, action } = response;

            const pending = this.pendingRequests.get(id);
            if (!pending) {
                return;
            }

            this.pendingRequests.delete(id);
            this.progressCallbacks.delete(id);

            if (success) {
                try {
                    // Handle different result types based on action
                    if (action === 'validate') {
                        // Validation result - return directly without reconstruction
                        pending.resolve(data);
                    } else {
                        // Pool generation result - reconstruct CTokenPool
                        const reconstructed = this._reconstructTokenPool(data);
                        pending.resolve(reconstructed);
                    }
                } catch (e) {
                    pending.reject(e);
                }
            } else {
                pending.reject(new Error(error.message || 'Unknown error'));
            }
        }
    }

    /**
     * Convert Array to ArrayBuffer
     * @param {Array<number>} arr
     * @returns {ArrayBuffer}
     */
    _arrayToBuffer(arr) {
        if (!arr || arr.length === 0) {
            return new ArrayBuffer(0);
        }
        return new Uint8Array(arr).buffer;
    }

    /**
     * Reconstruct CTokenPool from serialized worker data
     * @param {Object} data - Serialized pool data from worker
     * @returns {CTokenPool}
     */
    _reconstructTokenPool(data) {
        if (!data || data._type !== 'CTokenPool') {
            throw new Error('Invalid token pool data received from worker');
        }

        console.log('[TokenPoolGeneratorProxy] Reconstructing pool from worker data');
        console.log('[TokenPoolGeneratorProxy] Worker data received:');
        console.log('  data.dimensionDepth:', data.dimensionDepth);
        console.log('  data.dimensionsCount:', data.dimensionsCount);
        console.log('  data.totalValue:', data.totalValue);
        console.log('  data.friendlyID:', data.friendlyID);
        console.log('[TokenPoolGeneratorProxy] CTokenPool class available:', typeof CTokenPool);
        console.log('[TokenPoolGeneratorProxy] CTokenPoolBank class available:', typeof CTokenPoolBank);

        // Create pool instance
        const pool = new CTokenPool(
            null, // cryptoFactory (set separately if needed)
            data.dimensionsCount,
            this._arrayToBuffer(data.ownerID),
            this._arrayToBuffer(data.receiptID),
            1, // valuePerToken (calculated from totalValue/totalTokens)
            BigInt(data.totalValue),
            0, // currentIndex
            data.friendlyID,
            this._arrayToBuffer(data.masterSeedHash),
            new ArrayBuffer(0), // finalHash
            new ArrayBuffer(0)  // currentHash
        );

        // Explicitly set ALL properties to ensure user-configured values are applied
        // The constructor may calculate some values incorrectly due to hardcoded valuePerToken=1
        pool.mVersion = data.version;
        pool.mTokenPoolID = this._arrayToBuffer(data.tokenPoolID);
        pool.mStatus = data.status;
        pool.mDimensionDepth = BigInt(data.dimensionDepth);
        pool.mDimensionsCount = data.dimensionsCount;
        pool.mTotalValue = BigInt(data.totalValue);
        pool.mOwnerID = this._arrayToBuffer(data.ownerID);
        pool.mReceiptID = this._arrayToBuffer(data.receiptID);
        pool.mFriendlyID = data.friendlyID || '';
        pool.mMasterSeedHash = this._arrayToBuffer(data.masterSeedHash);

        if (data.pubKey && data.pubKey.length > 0) {
            pool.mPubKey = this._arrayToBuffer(data.pubKey);
        }

        // Reconstruct banks
        pool.mBanks = [];
        for (const bankData of data.banks) {
            const bank = new CTokenPoolBank(
                bankData.bankID,
                pool,
                this._arrayToBuffer(bankData.finalHash),
                bankData.currentDepth,
                this._arrayToBuffer(bankData.seedHash)
            );
            bank.mStatus = bankData.status;
            if (bankData.currentHash && bankData.currentHash.length > 0) {
                bank.mCurrentFinalHash = this._arrayToBuffer(bankData.currentHash);
            }
            pool.mBanks.push(bank);
        }

        // Set slot index if provided (for deterministic seed derivation tracking)
        if (data.slot !== undefined && data.slot !== null) {
            pool.setSlot(data.slot);
            console.log('[TokenPoolGeneratorProxy] Pool slot set:', data.slot);
        }

        console.log('[TokenPoolGeneratorProxy] Pool reconstructed');
        console.log('[TokenPoolGeneratorProxy] Pool is CTokenPool instance:', pool instanceof CTokenPool);
        console.log('[TokenPoolGeneratorProxy] Pool constructor name:', pool.constructor.name);
        console.log('[TokenPoolGeneratorProxy] Pool has getTokenPoolID:', 'getTokenPoolID' in pool);
        console.log('[TokenPoolGeneratorProxy] typeof pool.getTokenPoolID:', typeof pool.getTokenPoolID);

        return pool;
    }

    /**
     * Generate token pool in web worker with progress reporting
     * @param {Object} params - Pool generation parameters
     * @param {number} params.dimensionsCount - Number of banks
     * @param {BigInt|string} params.dimensionDepth - Tokens per bank
     * @param {BigInt|string} params.totalValue - Total value in GBUs
     * @param {ArrayBuffer|Uint8Array|Array} params.ownerID - Owner identifier
     * @param {ArrayBuffer|Uint8Array|Array} params.receiptID - Sacrifice receipt ID
     * @param {string} params.friendlyID - Pool name
     * @param {boolean} [params.verify=true] - Whether to verify hash chains
     * @param {Function} [progressCallback] - Called with progress updates
     * @param {number} [timeoutMs=300000] - Timeout in ms (default 5 minutes)
     * @returns {Promise<CTokenPool>}
     */
    async generatePool(params, progressCallback = null, timeoutMs = 300000) {
        if (!this.isReady) {
            await this.readyPromise;
        }

        const id = ++this.requestIdCounter;

        // Convert parameters to transferable format
        const workerParams = {
            dimensionsCount: params.dimensionsCount,
            dimensionDepth: params.dimensionDepth.toString(),
            totalValue: params.totalValue.toString(),
            ownerID: this._toTransferableArray(params.ownerID),
            receiptID: this._toTransferableArray(params.receiptID),
            friendlyID: params.friendlyID || '',
            tokenPoolID: this._toTransferableArray(params.tokenPoolID || crypto.getRandomValues(new Uint8Array(32))),
            verify: params.verify !== false,
            // Optional: Deterministic seed derived from keychain (if provided)
            // When not provided, worker will generate a random seed (legacy behavior)
            seed: params.seed ? this._toTransferableArray(params.seed) : null,
            // Optional: Slot index for tracking (passed through to reconstructed pool)
            slot: params.slot !== undefined ? params.slot : null
        };

        return new Promise((resolve, reject) => {
            // Set up timeout
            const timeoutHandle = setTimeout(() => {
                if (this.pendingRequests.has(id)) {
                    this.pendingRequests.delete(id);
                    this.progressCallbacks.delete(id);
                    reject(new Error(`Token pool generation timed out after ${timeoutMs}ms`));
                }
            }, timeoutMs);

            this.pendingRequests.set(id, {
                resolve: (result) => {
                    clearTimeout(timeoutHandle);
                    resolve(result);
                },
                reject: (error) => {
                    clearTimeout(timeoutHandle);
                    reject(error);
                }
            });

            if (progressCallback) {
                this.progressCallbacks.set(id, progressCallback);
            }

            this.worker.postMessage({
                action: 'generate',
                id: id,
                params: workerParams
            });
        });
    }

    /**
     * Convert various input types to transferable array format
     * @param {ArrayBuffer|Uint8Array|Array} input
     * @returns {Array<number>}
     */
    _toTransferableArray(input) {
        if (!input) {
            return [];
        }
        if (Array.isArray(input)) {
            return input;
        }
        if (input instanceof ArrayBuffer) {
            return Array.from(new Uint8Array(input));
        }
        if (input instanceof Uint8Array) {
            return Array.from(input);
        }
        return [];
    }

    /**
     * Validate token pool seed by recomputing hash chains in web worker
     * This verifies that a regenerated seed produces the correct final hashes
     * @param {Object} params - Validation parameters
     * @param {ArrayBuffer|Uint8Array|Array} params.seed - Master seed to validate (32 bytes)
     * @param {Array<Object>} params.banks - Bank data with finalHash for each bank
     * @param {number} params.dimensionDepth - Number of hashes per bank
     * @param {number} params.dimensionsCount - Number of banks
     * @param {Function} [progressCallback] - Called with progress updates
     * @param {number} [timeoutMs=300000] - Timeout in ms (default 5 minutes)
     * @returns {Promise<Object>} - Validation result { valid, validBanks, totalBanks, errors }
     */
    async validatePool(params, progressCallback = null, timeoutMs = 300000) {
        if (!this.isReady) {
            await this.readyPromise;
        }

        const id = ++this.requestIdCounter;

        // Convert parameters to transferable format
        const workerParams = {
            seed: this._toTransferableArray(params.seed),
            banks: params.banks.map(bank => ({
                bankID: bank.bankID,
                finalHash: this._toTransferableArray(bank.finalHash)
            })),
            dimensionDepth: params.dimensionDepth.toString(),
            dimensionsCount: params.dimensionsCount
        };

        return new Promise((resolve, reject) => {
            // Set up timeout
            const timeoutHandle = setTimeout(() => {
                if (this.pendingRequests.has(id)) {
                    this.pendingRequests.delete(id);
                    this.progressCallbacks.delete(id);
                    reject(new Error(`Token pool validation timed out after ${timeoutMs}ms`));
                }
            }, timeoutMs);

            this.pendingRequests.set(id, {
                resolve: (result) => {
                    clearTimeout(timeoutHandle);
                    resolve(result);
                },
                reject: (error) => {
                    clearTimeout(timeoutHandle);
                    reject(error);
                }
            });

            if (progressCallback) {
                this.progressCallbacks.set(id, progressCallback);
            }

            this.worker.postMessage({
                action: 'validate',
                id: id,
                params: workerParams
            });
        });
    }

    /**
     * Check if worker is ready
     * @returns {boolean}
     */
    get ready() {
        return this.isReady;
    }

    /**
     * Terminate worker (for cleanup)
     */
    terminate() {
        if (this.worker) {
            this.worker.terminate();
            this.worker = null;
            this.isReady = false;
            this.pendingRequests.clear();
            this.progressCallbacks.clear();
            TokenPoolGeneratorProxy._instance = null;
        }
    }
}

// Export singleton instance getter
export default TokenPoolGeneratorProxy.getInstance;
