/**
 * @file TokenPoolGeneratorWorker.js
 * @brief Web Worker for offloading Token Pool generation from main thread
 * @version 1.0
 * @date 2025
 *
 * This worker handles computationally intensive token pool generation
 * (hash chain computation) to prevent blocking the main thread.
 * Reports progress during generation for UI feedback.
 */

'use strict';

/**
 * SHA-256 hash using Web Crypto API (async but efficient)
 * @param {ArrayBuffer|Uint8Array} data - Data to hash
 * @returns {Promise<ArrayBuffer>} - 32-byte hash
 */
async function sha256(data) {
    const buffer = data instanceof ArrayBuffer ? data : data.buffer;
    return await crypto.subtle.digest('SHA-256', buffer);
}

/**
 * Generate random 32-byte vector
 * @returns {ArrayBuffer}
 */
function getRandomVector(size) {
    const array = new Uint8Array(size);
    crypto.getRandomValues(array);
    return array.buffer;
}

/**
 * Compare two ArrayBuffers for equality
 * @param {ArrayBuffer} a
 * @param {ArrayBuffer} b
 * @returns {boolean}
 */
function compareByteVectors(a, b) {
    if (a.byteLength !== b.byteLength) return false;
    const viewA = new Uint8Array(a);
    const viewB = new Uint8Array(b);
    for (let i = 0; i < viewA.length; i++) {
        if (viewA[i] !== viewB[i]) return false;
    }
    return true;
}

/**
 * Convert ArrayBuffer to transferable format (Array)
 * @param {ArrayBuffer} buffer
 * @returns {Array<number>}
 */
function bufferToArray(buffer) {
    return Array.from(new Uint8Array(buffer));
}

/**
 * Generate token pool dimensions (banks) with progress reporting and ETA
 * @param {Object} params - Pool parameters
 * @param {string} requestId - Request ID for progress updates
 * @returns {Object} - Serialized pool data
 */
async function generatePoolDimensions(params, requestId) {
    const {
        dimensionsCount,
        dimensionDepth,
        totalValue,
        ownerID,
        receiptID,
        friendlyID,
        tokenPoolID,
        verify,
        seed,  // Optional: Deterministic seed from keychain
        slot   // Optional: Slot index for tracking
    } = params;

    // Use provided seed (deterministic) or generate random seed (legacy)
    let masterSeedHash;
    if (seed && seed.length === 32) {
        // Convert seed array back to ArrayBuffer
        masterSeedHash = new Uint8Array(seed).buffer;
        console.log('[TokenPoolWorker] Using deterministic seed from keychain');
    } else {
        // Legacy behavior: generate random seed
        masterSeedHash = getRandomVector(32);
        console.warn('[TokenPoolWorker] Using random seed - seed will be LOST after on-chain registration!');
    }
    let masterNr = masterSeedHash;

    const banks = [];
    const totalBankOperations = dimensionsCount * (verify ? 2 : 1);
    let completedBankOperations = 0;

    // Timing for ETA calculation
    const startTime = performance.now();
    let lastProgressTime = startTime;

    // Progress reporting interval (report every bank or at minimum every 500ms)
    const minReportIntervalMs = 500;
    let lastProgressReport = -1;

    const temp = new Uint8Array(64);
    const depthNum = Number(dimensionDepth);
    const totalHashOperations = dimensionsCount * depthNum * (verify ? 2 : 1);
    let completedHashOperations = 0;

    for (let bankIndex = 0; bankIndex < dimensionsCount; bankIndex++) {
        // Dimension Switch - generate unique seed for each bank
        const masterArray = new Uint8Array(masterNr);
        temp.set(masterArray, 0);
        temp.set(masterArray, 32);

        masterNr = await sha256(temp.buffer);
        const bankSeed = await sha256(masterNr);

        // Generate hash chain for this bank
        let currentHash = bankSeed;

        // Forward hash chain generation (find ending hash)
        for (let i = 0; i < depthNum; i++) {
            currentHash = await sha256(currentHash);
            completedHashOperations++;

            // Yield to event loop periodically and report progress
            if (i % 500 === 0 && i > 0) {
                await new Promise(resolve => setTimeout(resolve, 0));

                // Report intra-bank progress for large banks
                const now = performance.now();
                if (now - lastProgressTime >= minReportIntervalMs) {
                    const progress = Math.round((completedHashOperations / totalHashOperations) * 100);
                    const elapsed = now - startTime;
                    const estimatedTotal = progress > 0 ? (elapsed / progress) * 100 : 0;
                    const etaMs = Math.max(0, estimatedTotal - elapsed);

                    self.postMessage({
                        type: 'progress',
                        requestId: requestId,
                        data: {
                            progress: progress,
                            currentBank: bankIndex + 1,
                            totalBanks: dimensionsCount,
                            currentHashInBank: i,
                            hashesPerBank: depthNum,
                            phase: 'generating',
                            elapsedMs: Math.round(elapsed),
                            etaMs: Math.round(etaMs),
                            hashRate: Math.round(completedHashOperations / (elapsed / 1000))
                        }
                    });
                    lastProgressTime = now;
                }
            }
        }
        const endingHash = currentHash;

        // Verification pass (optional but recommended)
        if (verify) {
            currentHash = bankSeed;
            for (let i = 0; i < depthNum; i++) {
                currentHash = await sha256(currentHash);
                completedHashOperations++;

                if (i % 500 === 0 && i > 0) {
                    await new Promise(resolve => setTimeout(resolve, 0));

                    const now = performance.now();
                    if (now - lastProgressTime >= minReportIntervalMs) {
                        const progress = Math.round((completedHashOperations / totalHashOperations) * 100);
                        const elapsed = now - startTime;
                        const estimatedTotal = progress > 0 ? (elapsed / progress) * 100 : 0;
                        const etaMs = Math.max(0, estimatedTotal - elapsed);

                        self.postMessage({
                            type: 'progress',
                            requestId: requestId,
                            data: {
                                progress: progress,
                                currentBank: bankIndex + 1,
                                totalBanks: dimensionsCount,
                                currentHashInBank: i,
                                hashesPerBank: depthNum,
                                phase: 'verifying',
                                elapsedMs: Math.round(elapsed),
                                etaMs: Math.round(etaMs),
                                hashRate: Math.round(completedHashOperations / (elapsed / 1000))
                            }
                        });
                        lastProgressTime = now;
                    }
                }
            }

            if (!compareByteVectors(currentHash, endingHash)) {
                throw new Error(`Bank ${bankIndex} verification failed`);
            }
            completedBankOperations++;
        }

        // Store bank data
        banks.push({
            bankID: bankIndex,
            finalHash: bufferToArray(endingHash),
            seedHash: bufferToArray(bankSeed),
            currentDepth: 0,
            currentHash: bufferToArray(new ArrayBuffer(0)),
            status: 0 // active
        });

        completedBankOperations++;

        // Report progress after each bank completion
        const now = performance.now();
        const progress = Math.round((completedHashOperations / totalHashOperations) * 100);
        const elapsed = now - startTime;
        const estimatedTotal = progress > 0 ? (elapsed / progress) * 100 : 0;
        const etaMs = Math.max(0, estimatedTotal - elapsed);

        self.postMessage({
            type: 'progress',
            requestId: requestId,
            data: {
                progress: progress,
                currentBank: bankIndex + 1,
                totalBanks: dimensionsCount,
                currentHashInBank: depthNum,
                hashesPerBank: depthNum,
                phase: verify ? 'verified' : 'generated',
                bankComplete: true,
                elapsedMs: Math.round(elapsed),
                etaMs: Math.round(etaMs),
                hashRate: Math.round(completedHashOperations / (elapsed / 1000))
            }
        });
        lastProgressTime = now;
        lastProgressReport = bankIndex;
    }

    // Return serialized pool data
    return {
        _type: 'CTokenPool',
        version: 2,
        ownerID: ownerID,
        masterSeedHash: bufferToArray(masterSeedHash),
        dimensionDepth: dimensionDepth.toString(),
        dimensionsCount: dimensionsCount,
        totalValue: totalValue.toString(),
        receiptID: receiptID,
        tokenPoolID: tokenPoolID,
        friendlyID: friendlyID,
        status: 0, // active
        pubKey: [],
        banks: banks,
        // Pass slot index back for tracking deterministic seed derivation
        slot: slot !== undefined && slot !== null ? slot : null
    };
}

/**
 * Handle incoming generation request
 * @param {Object} message - { id, action, params }
 */
async function handleGenerateRequest(message) {
    const { id, params } = message;

    try {
        // Report start
        self.postMessage({
            type: 'progress',
            requestId: id,
            data: {
                progress: 0,
                currentBank: 0,
                totalBanks: params.dimensionsCount,
                phase: 'initializing'
            }
        });

        const result = await generatePoolDimensions(params, id);

        self.postMessage({
            type: 'result',
            id: id,
            success: true,
            data: result
        });

    } catch (error) {
        self.postMessage({
            type: 'result',
            id: id,
            success: false,
            error: {
                message: error?.message || String(error),
                stack: error?.stack || 'No stack trace'
            }
        });
    }
}

/**
 * Validate token pool seed by recomputing hash chains and comparing final hashes
 * @param {Object} params - Validation parameters
 * @param {string} requestId - Request ID for progress updates
 * @returns {Object} - Validation result { valid, validBanks, totalBanks, errors, bankResults }
 */
async function validatePoolSeed(params, requestId) {
    const {
        seed,
        banks,
        dimensionDepth,
        dimensionsCount
    } = params;

    // Convert seed array to ArrayBuffer
    const masterSeedHash = new Uint8Array(seed).buffer;
    let masterNr = masterSeedHash;

    const depthNum = Number(dimensionDepth);
    const totalHashOperations = dimensionsCount * depthNum;
    let completedHashOperations = 0;

    const errors = [];
    const bankResults = [];
    let validBanks = 0;

    // Timing for ETA calculation
    const startTime = performance.now();
    let lastProgressTime = startTime;
    const minReportIntervalMs = 500;

    const temp = new Uint8Array(64);

    for (let bankIndex = 0; bankIndex < dimensionsCount; bankIndex++) {
        // Dimension Switch - generate unique seed for each bank (same algorithm as generation)
        const masterArray = new Uint8Array(masterNr);
        temp.set(masterArray, 0);
        temp.set(masterArray, 32);

        masterNr = await sha256(temp.buffer);
        const bankSeed = await sha256(masterNr);

        // Generate hash chain for this bank
        let currentHash = bankSeed;

        // Forward hash chain generation (compute ending hash)
        for (let i = 0; i < depthNum; i++) {
            currentHash = await sha256(currentHash);
            completedHashOperations++;

            // Yield to event loop periodically and report progress
            if (i % 500 === 0 && i > 0) {
                await new Promise(resolve => setTimeout(resolve, 0));

                const now = performance.now();
                if (now - lastProgressTime >= minReportIntervalMs) {
                    const progress = Math.round((completedHashOperations / totalHashOperations) * 100);
                    const elapsed = now - startTime;
                    const estimatedTotal = progress > 0 ? (elapsed / progress) * 100 : 0;
                    const etaMs = Math.max(0, estimatedTotal - elapsed);

                    self.postMessage({
                        type: 'progress',
                        requestId: requestId,
                        data: {
                            progress: progress,
                            currentBank: bankIndex + 1,
                            totalBanks: dimensionsCount,
                            currentHashInBank: i,
                            hashesPerBank: depthNum,
                            phase: 'validating',
                            elapsedMs: Math.round(elapsed),
                            etaMs: Math.round(etaMs),
                            hashRate: Math.round(completedHashOperations / (elapsed / 1000)),
                            validatedBanks: validBanks,
                            failedBanks: bankIndex - validBanks
                        }
                    });
                    lastProgressTime = now;
                }
            }
        }

        const computedFinalHash = currentHash;

        // Get expected final hash from bank data
        const expectedBank = banks.find(b => b.bankID === bankIndex);
        const expectedFinalHash = expectedBank ? new Uint8Array(expectedBank.finalHash).buffer : null;

        // Compare computed vs expected
        let bankValid = false;
        if (expectedFinalHash && compareByteVectors(computedFinalHash, expectedFinalHash)) {
            validBanks++;
            bankValid = true;
        } else {
            errors.push({
                bankID: bankIndex,
                message: expectedFinalHash
                    ? `Bank ${bankIndex}: Final hash mismatch`
                    : `Bank ${bankIndex}: No expected hash provided`
            });
        }

        bankResults.push({
            bankID: bankIndex,
            valid: bankValid,
            computedHash: bufferToArray(computedFinalHash),
            expectedHash: expectedFinalHash ? bufferToArray(expectedFinalHash) : null
        });

        // Report progress after each bank
        const now = performance.now();
        const progress = Math.round((completedHashOperations / totalHashOperations) * 100);
        const elapsed = now - startTime;
        const estimatedTotal = progress > 0 ? (elapsed / progress) * 100 : 0;
        const etaMs = Math.max(0, estimatedTotal - elapsed);

        self.postMessage({
            type: 'progress',
            requestId: requestId,
            data: {
                progress: progress,
                currentBank: bankIndex + 1,
                totalBanks: dimensionsCount,
                currentHashInBank: depthNum,
                hashesPerBank: depthNum,
                phase: 'validating',
                bankComplete: true,
                bankValid: bankValid,
                elapsedMs: Math.round(elapsed),
                etaMs: Math.round(etaMs),
                hashRate: Math.round(completedHashOperations / (elapsed / 1000)),
                validatedBanks: validBanks,
                failedBanks: bankResults.filter(b => !b.valid).length
            }
        });
        lastProgressTime = now;
    }

    return {
        valid: validBanks === dimensionsCount,
        validBanks: validBanks,
        totalBanks: dimensionsCount,
        errors: errors,
        bankResults: bankResults,
        elapsedMs: Math.round(performance.now() - startTime)
    };
}

/**
 * Handle incoming validation request
 * @param {Object} message - { id, action, params }
 */
async function handleValidateRequest(message) {
    const { id, params } = message;

    try {
        // Report start
        self.postMessage({
            type: 'progress',
            requestId: id,
            data: {
                progress: 0,
                currentBank: 0,
                totalBanks: params.dimensionsCount,
                phase: 'initializing',
                validatedBanks: 0,
                failedBanks: 0
            }
        });

        const result = await validatePoolSeed(params, id);

        self.postMessage({
            type: 'result',
            id: id,
            action: 'validate',
            success: true,
            data: result
        });

    } catch (error) {
        self.postMessage({
            type: 'result',
            id: id,
            action: 'validate',
            success: false,
            error: {
                message: error?.message || String(error),
                stack: error?.stack || 'No stack trace'
            }
        });
    }
}

/**
 * Worker message handler
 */
self.onmessage = function(event) {
    const message = event.data;

    if (message.action === 'generate') {
        handleGenerateRequest(message);
    } else if (message.action === 'validate') {
        handleValidateRequest(message);
    } else {
        self.postMessage({
            type: 'result',
            id: message.id,
            success: false,
            error: {
                message: `Unknown action: ${message.action}`
            }
        });
    }
};

// Signal worker is ready
self.postMessage({ type: 'ready' });
