/**
 * GRIDNET Blockchain Explorer UI dApp
 * 
 * A modern, maintainable implementation of the blockchain explorer for GRIDNET OS.
 * This implementation follows a modular approach with clear separation of concerns.
 */

//Imports - BEGIN
import { CWindow } from "/lib/window.js"
import { CVMMetaSection, CVMMetaEntry, CVMMetaGenerator, CVMMetaParser } from '/lib/MetaData.js'
import { CNetMsg } from '/lib/NetMsg.js'
import { CTools, CDataConcatenator } from '/lib/tools.js'
import { CAppSettings, CSettingsManager } from "/lib/SettingsManager.js"
import { CContentHandler } from "/lib/AppSelector.js"
import { CSearchFilter } from "/lib/SearchFilter.js"
import CVMContext from '/lib/VMContext.js'
//Imports - END

// Import blockchain entity classes
import { CBlockDesc } from '/lib/BlockDesc.js'
import { CTransactionDesc } from '/lib/TransactionDesc.js'
import { CDomainDesc } from '/lib/DomainDesc.js'
import { CSearchResults } from '/lib/SearchResults.js'
import { COperatorSecurityInfo } from '/lib/OperatorSecurityInfo.js'

/**
 * Enum Initializer - Handles both system and fallback enums
 */
class EnumInitializer {
    /**
     * Initialize enums from system or use fallbacks
     * @returns {Object} Object containing all necessary enums
     */
    static initialize() {
        let enums = {};
        
        try {
            // Try to import enums from the system
            enums.eSearchResultElemType = eSearchResultElemType;
            enums.eSortBlocksBy = eSortBlocksBy;
            enums.eSortTransactionsBy = eSortTransactionsBy;
            enums.eTransactionValidationResult = eTransactionValidationResult;
            enums.eBlockType = eBlockType;
            enums.eTXType = eTXType;
            enums.eLivenessState = eLivenessState;
            enums.eVMMetaCodeExecutionMode = eVMMetaCodeExecutionMode;
        } catch (e) {
            console.warn("Error importing enums from system:", e);
            
            // Define fallback enums
            enums.eVMMetaCodeExecutionMode = {
                RAW: 1,
                FORMATTED: 2,
                GUI: 3
            };
            
            enums.eSearchResultElemType = {
                TRANSACTION: 0,
                BLOCK: 1,
                DOMAIN: 2
            };
            
            enums.eSortBlocksBy = {
                heightAsc: 0,
                heightDesc: 1,
                timestampAsc: 2,
                timestampDesc: 3
            };
            
            enums.eSortTransactionsBy = {
                heightAsc: 0,
                heightDesc: 1,
                timestampAsc: 2,
                timestampDesc: 3,
                valueAsc: 4,
                valueDesc: 5
            };
            
            enums.eBlockchainStatType = {
                STATUS: 0,
                NETWORK_UTILIZATION: 1,
                BLOCK_SIZE: 2,
                BLOCK_REWARDS: 3,
                BLOCK_TIME: 4,
                KEY_BLOCK_TIME: 5,
                DAILY_STATS: 6,
                LIVENESS: 7,
                PRICE: 8,
                HEIGHT: 9,
                KEY_HEIGHT: 10
            };
            
            enums.eTransactionValidationResult = {
                VALID: 0,
                INVALID: 1,
                UNKNOWN_ISSUER: 2,
                INSUFFICIENT_ERG: 3,
                ERG_BID_TOO_LOW: 4,
                PUB_NOT_MATCH: 5,
                NO_ID_TOKEN: 6,
                NO_PUBLIC_KEY: 7,
                INVALID_SIG: 8,
                INVALID_BALANCE: 9,
                INCONSISTENT_DATA: 10,
                VALID_NO_TRIE_EFFECT: 11,
                INVALID_SACRIFICE: 12,
                INVALID_NONCE: 13,
                FORKED_OUT: 14
            };
            
            enums.eBlockType = {
                DATA_BLOCK: 0,
                KEY_BLOCK: 1
            };
            
            enums.eTXType = {
                TRANSFER: 0,
                BLOCK_REWARD: 1,
                OFF_CHAIN: 2,
                OFF_CHAIN_CASH_OUT: 3,
                CONTRACT: 4,
                RANDOM: 99
            };
            
            enums.eLivenessState = {
                NO_LIVENESS: 0,
                LOW_LIVENESS: 1,
                MEDIUM_LIVENESS: 2,
                HIGH_LIVENESS: 3
            };
        }
        
        // Define eRequestType locally for tracking pending requests
        enums.eRequestType = {
            SEARCH: 0,
            RECENT_BLOCKS: 1,
            RECENT_TRANSACTIONS: 2,
            BLOCK_DETAILS: 3,
            TRANSACTION_DETAILS: 4,
            DOMAIN_DETAILS: 5,
            DOMAIN_HISTORY: 6,
            BLOCKCHAIN_STATUS: 7,
            NETWORK_UTILIZATION: 8,
            BLOCK_SIZE: 9,
            BLOCK_REWARDS: 10,
            AVERAGE_BLOCK_TIME: 11,
            AVERAGE_KEY_BLOCK_TIME: 12,
            TRANSACTION_DAILY_STATS: 13,
            LIVENESS: 14,
            USDT_PRICE: 15,
            HEIGHT: 16,
            KEY_HEIGHT: 17
        };
        
        return enums;
    }
}

/**
 * Main CUIBlockchainExplorer class
 * Extends CWindow to provide a blockchain explorer UI application.
 * This class coordinates the UI, data management, and window lifecycle.
 */
class CUIBlockchainExplorer extends CWindow {
    /**
     * Static settings
     */
    static sCurrentSettings = null;

    /**
     * Construct a CUIBlockchainExplorer instance
     * @param {number} positionX - Window X position
     * @param {number} positionY - Window Y position
     * @param {number} width - Window width
     * @param {number} height - Window height
     * @param {*} data - Window data
     * @param {*} dataType - Window data type
     * @param {string} filePath - File path
     * @param {*} thread - Thread
     */
    constructor(positionX, positionY, width, height, data, dataType, filePath, thread) {
        super(positionX, positionY, width, height, blockchainExplorerBody, "Experimental Blockchain Explorer (EBE)", CUIBlockchainExplorer.getIcon(), true);
        
        // Initialize tools
        this.mTools = CTools.getInstance();
        
        // Initialize system or fallback enums
        this.enums = EnumInitializer.initialize();
        
        // Initialize metadata parser
        this.mMetaParser = new CVMMetaParser();
        
        // Initialize data manager with callbacks for updates and errors
        this.dataManager = new BlockchainDataManager(
            this.enums,
            this.handleDataUpdate.bind(this),
            this.handleError.bind(this)
        );
        
        // Track window state
        this.mLastHeightRearangedAt = 0;
        this.mLastWidthRearangedAt = 0;
        this.mErrorMsg = "";
        this.initialized = false;
        this.subscriptionActive = false;
        
        // Set up controller thread
        this.mControllerThreadInterval = 10000; // 10 seconds
        this.mControlerExecuting = false;
        this.mControler = 0;
    }
    
    /**
     * Get file handlers for the application
     * @returns {Array<CContentHandler>} Content handlers
     */
    static getFileHandlers() {
        return [new CContentHandler(null, eDataType.bytes, this.getPackageID())];
    }
    
    /**
     * Get the package ID
     * @returns {string} Package ID
     */
    static getPackageID() {
        return "org.gridnetproject.UIdApps.BlockchainExplorer";
    }
    
    /**
     * Get the default category
     * @returns {string} Default category
     */
    static getDefaultCategory() {
        return 'explore'; // Using the 'explore' category as this is an explorer app
    }
    
    /**
     * Get the application icon
     * @returns {string} Icon data URL
     */
    static getIcon() {
        return `data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMjQiIGhlaWdodD0iMjQiIHZpZXdCb3g9IjAgMCAyNCAyNCIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIiBmaWxsPSIjM2Q2ZWZmIiBzdHJva2U9IiMzZDZlZmYiIHN0cm9rZS13aWR0aD0iMCIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIiBzdHJva2UtbGluZWpvaW49InJvdW5kIj48cGF0aCBkPSJNMTEgMTJhMSAxIDAgMCAxIDIgMGgtMnptLTEtNC4yNUExIDEgMCAwIDEgMTEgN2gyYTEgMSAwIDAgMSAxIC43NXYuNWgtNHYtLjV6TTggMTJ2LTFhMSAxIDAgMCAxIDEtMWg2YTEgMSAwIDAgMSAxIDF2MWE3IDcgMCAxIDEtOCAwem03IDBhNiA2IDAgMSAxLTEyIDA2IDYgMCAwIDEgMTIgMHptLTIgMGExIDEgMCAwIDEtMiAwaDJabS43IDQuMjVhMSAxIDAgMCAxLTEgLjc1aC0yYTEgMSAwIDAgMS0xLS43NXYtLjVoNHYuNXpNNyAxNGExIDEgMCAwIDEgMS0xaDhhMSAxIDAgMCAxIDEgMXYxYTEgMSAwIDAgMS0xIDFIOGExIDEgMCAwIDEtMS0xdi0xeiIvPjwvc3ZnPg==`;
    }
    
    /**
     * Get application settings
     * @returns {CAppSettings} Application settings
     */
    static getSettings() {
        return CUIBlockchainExplorer.sCurrentSettings;
    }
    
    /**
     * Set application settings
     * @param {CAppSettings} sets - Settings to set
     * @returns {boolean} Success status
     */
    static setSettings(sets) {
        if (!(sets instanceof CAppSettings))
            return false;
            
        CUIBlockchainExplorer.sCurrentSettings = sets;
        return true;
    }
    
    /**
     * Load settings from settings manager
     * @returns {boolean} Success status
     */
    loadSettings() {
        CVMContext.getInstance().getSettingsManager.loadSettings(CUIBlockchainExplorer.getPackageID());
        return this.activateSetings();
    }
    
    /**
     * Activate loaded settings
     * @returns {boolean} Success status
     */
    activateSetings() {
        let sets = CUIBlockchainExplorer.getSettings();
        let settings = null;
        
        if (sets == null || typeof sets.getVersion === 'undefined')
            return false;
            
        if (sets.getVersion == 1) {
            settings = sets.getData;
            if (settings == null || typeof settings === 'undefined') {
                sets = CUIBlockchainExplorer.getDefaultSettings();
                settings = sets.getData;
            }
        } else return false;
        
        return true;
    }
    
    /**
     * Save settings to settings manager
     */
    saveSettings() {
        let sets = CUIBlockchainExplorer.getSettings();
        CVMContext.getInstance().getSettingsManager.saveAppSettings(sets);
    }
    
    /**
     * Get default settings
     * @returns {CAppSettings} Default settings
     */
    static getDefaultSettings() {
        let obj = {
            version: 1,
            refreshInterval: 10000, // 10 seconds
            defaultPageSize: 10,
            showAdvancedStats: false
        };
        return new CAppSettings(CUIBlockchainExplorer.getPackageID(), obj);
    }
    
    /**
     * Initialize UI components
     */
    initialize() {
        if (this.initialized) return;
        
        // Load settings
        if (this.loadSettings()) {
            this.mTools.logEvent('['+this.getPackageID() + ']\'s settings activated!',
                eLogEntryCategory.dApp, 0, eLogEntryType.notification);
        } else {
            CUIBlockchainExplorer.setSettings(this.getDefaultSettings());
            this.mTools.logEvent('Failed to activate provided settings for ['+this.getPackageID() + ']. Assuming defaults!',
                eLogEntryCategory.dApp, 0, eLogEntryType.failure);
        }
        
        // Initialize UI elements
        this.initUIElements();
        
        // Initialize data manager with event listeners
        this.dataManager.initEventListeners(this.getWinID());
        
        // Import and initialize the search configuration panel
        import('./SearchConfigPanel.js').then(module => {
            const SearchConfigPanel = module.default;
            
            // Create container for search config panel
            const searchConfigContainer = document.createElement('div');
            searchConfigContainer.id = 'search-config-container';
            searchConfigContainer.style.display = 'none'; // Hidden by default
            
            // Add it after the search section
            const searchSection = this.getWindowDocument().querySelector('.search-section');
            searchSection.parentNode.insertBefore(searchConfigContainer, searchSection.nextSibling);
            
            // Initialize the search config panel
            this.searchConfigPanel = new SearchConfigPanel(
                searchConfigContainer,
                this.enums,
                this.onSearchConfigChange.bind(this)
            );
            
            // Add toggle button to search section
            const toggleButton = document.createElement('button');
            toggleButton.className = 'search-button';
            toggleButton.textContent = 'Advanced';
            toggleButton.style.marginLeft = '0.5rem';
            toggleButton.addEventListener('click', () => {
                const isVisible = searchConfigContainer.style.display !== 'none';
                searchConfigContainer.style.display = isVisible ? 'none' : 'block';
                toggleButton.textContent = isVisible ? 'Advanced' : 'Simple';
            });
            
            // Add button to search section
            const searchButton = this.getWindowDocument().querySelector('#search-button');
            searchButton.parentNode.insertBefore(toggleButton, searchButton.nextSibling);
        }).catch(error => {
            console.error("Error loading search configuration panel:", error);
        });
        
        // Initialize UI
        this.ui = new BlockchainExplorerUI(this.elements, this.enums);
        this.ui.registerCallbacks({
            onSearch: this.performSearch.bind(this),
            onDomainSearch: this.searchDomain.bind(this),
            onViewBlockDetails: this.viewBlockDetails.bind(this),
            onViewTransactionDetails: this.viewTransactionDetails.bind(this),
            onSectionChange: this.onSectionChange.bind(this),
            onError: this.handleError.bind(this)
        });
        this.ui.initialize();
        
        // Start controller thread
        this.mControler = CVMContext.getInstance().createJSThread(this.controllerThreadF.bind(this), this.getProcessID(), this.mControllerThreadInterval);
        
        // Fetch initial data
        this.fetchInitialData();
        
        this.initialized = true;
    }
    
    /**
     * Handle search configuration changes
     * @param {Object} config - Search configuration
     */
    onSearchConfigChange(config) {
        // Store the current search configuration
        this.currentSearchConfig = config;
    }
    
    /**
     * Initialize UI element references
     */
    initUIElements() {
        // Get references to DOM elements
        this.elements = {
            loadingOverlay: this.getWindowDocument().getElementById('loading-overlay'),
            sidebar: this.getWindowDocument().getElementById('sidebar'),
            navItems: this.getWindowDocument().querySelectorAll('.nav-item'),
            contentSections: this.getWindowDocument().querySelectorAll('.content-section'),
            searchInput: this.getWindowDocument().getElementById('search-input'),
            searchButton: this.getWindowDocument().getElementById('search-button'),
            domainSearchInput: this.getWindowDocument().getElementById('domain-search-input'),
            domainSearchButton: this.getWindowDocument().getElementById('domain-search-button'),
            blockchainStatus: this.getWindowDocument().getElementById('blockchain-status'),
            blockchainHeight: this.getWindowDocument().getElementById('blockchain-height'),
            usdtPrice: this.getWindowDocument().getElementById('usdt-price'),
            livenessIndicator: this.getWindowDocument().getElementById('liveness-indicator'),
            networkUtilization: this.getWindowDocument().getElementById('network-utilization'),
            avgBlockSize: this.getWindowDocument().getElementById('avg-block-size'),
            blockRewards: this.getWindowDocument().getElementById('block-rewards'),
            avgBlockTime: this.getWindowDocument().getElementById('avg-block-time'),
            keyBlockTime: this.getWindowDocument().getElementById('key-block-time'),
            networkUtil24h: this.getWindowDocument().getElementById('network-util-24h'),
            blockRewards24h: this.getWindowDocument().getElementById('block-rewards-24h'),
            blockSize24h: this.getWindowDocument().getElementById('block-size-24h')
        };
    }
    
    /**
     * Handle data updates from data manager
     * @param {number} type - Update type
     * @param {*} data - Updated data
     */
    handleDataUpdate(type, data) {
        // Update UI based on the update type
        switch (type) {
            case this.enums.eRequestType.BLOCKCHAIN_STATUS:
            case this.enums.eRequestType.LIVENESS:
            case this.enums.eRequestType.HEIGHT:
            case this.enums.eRequestType.KEY_HEIGHT:
            case this.enums.eRequestType.USDT_PRICE:
                this.ui.updateStatusDisplay({
                    blockchainStatus: type === this.enums.eRequestType.BLOCKCHAIN_STATUS ? data : undefined,
                    height: type === this.enums.eRequestType.HEIGHT ? data : undefined,
                    keyHeight: type === this.enums.eRequestType.KEY_HEIGHT ? data : undefined,
                    usdtPrice: type === this.enums.eRequestType.USDT_PRICE ? data : undefined,
                    liveness: type === this.enums.eRequestType.LIVENESS ? data : undefined
                });
                break;
                
            case this.enums.eRequestType.NETWORK_UTILIZATION:
            case this.enums.eRequestType.BLOCK_SIZE:
            case this.enums.eRequestType.BLOCK_REWARDS:
            case this.enums.eRequestType.AVERAGE_BLOCK_TIME:
                this.ui.updateDashboardDisplay({
                    networkUtilization: type === this.enums.eRequestType.NETWORK_UTILIZATION ? data : undefined,
                    blockSize: type === this.enums.eRequestType.BLOCK_SIZE ? data : undefined,
                    blockRewards: type === this.enums.eRequestType.BLOCK_REWARDS ? data : undefined,
                    avgBlockTime: type === this.enums.eRequestType.AVERAGE_BLOCK_TIME ? data : undefined
                });
                this.ui.updateStatisticsDisplay({
                    networkUtilization: type === this.enums.eRequestType.NETWORK_UTILIZATION ? data : undefined,
                    blockSize: type === this.enums.eRequestType.BLOCK_SIZE ? data : undefined,
                    blockRewards: type === this.enums.eRequestType.BLOCK_REWARDS ? data : undefined
                });
                break;
                
            case this.enums.eRequestType.AVERAGE_KEY_BLOCK_TIME:
                this.ui.updateStatisticsDisplay({
                    avgKeyBlockTime: data
                });
                break;
                
            case this.enums.eRequestType.RECENT_BLOCKS:
                if (this.ui.tables.recentBlocks) {
                    this.ui.tables.recentBlocks.setData(data);
                }
                if (this.ui.tables.blocks) {
                    this.ui.tables.blocks.setData(data);
                }
                break;
                
            case this.enums.eRequestType.RECENT_TRANSACTIONS:
                if (this.ui.tables.recentTransactions) {
                    this.ui.tables.recentTransactions.setData(data);
                }
                if (this.ui.tables.transactions) {
                    this.ui.tables.transactions.setData(data);
                }
                break;
                
            case this.enums.eRequestType.TRANSACTION_DAILY_STATS:
                this.ui.updateTransactionsChart(data);
                this.ui.updateDailyStatsChart(data);
                break;
                
            case this.enums.eRequestType.BLOCK_DETAILS:
                this.ui.displayBlockDetails(data);
                break;
                
            case this.enums.eRequestType.TRANSACTION_DETAILS:
                this.ui.displayTransactionDetails(data);
                break;
                
            case this.enums.eRequestType.DOMAIN_DETAILS:
                this.ui.displayDomainDetails(data);
                break;
                
            case this.enums.eRequestType.DOMAIN_HISTORY:
                if (this.ui.tables.domainHistory) {
                    this.ui.tables.domainHistory.setData(data);
                }
                break;
                
            case this.enums.eRequestType.SEARCH:
                this.ui.displaySearchResults(data, this.lastSearchQuery);
                break;
        }
    }
    
    /**
     * Handle errors
     * @param {Error|string} error - Error to handle
     */
    handleError(error) {
        const message = typeof error === 'string' ? error : error.message;
        this.mErrorMsg = message;
        this.askString('Error', message, () => {}, false);
    }
    
    /**
     * Fetch initial application data
     */
    async fetchInitialData() {
        this.ui.showLoading(true);
        
        try {
            // First fetch blockchain status
            await this.dataManager.requestBlockchainStatus();
            
            // Then fetch other data in parallel
            await Promise.all([
                this.dataManager.requestLiveness(),
                this.dataManager.requestHeight(),
                this.dataManager.requestUSDTPrice(),
                this.dataManager.requestRecentBlocks(),
                this.dataManager.requestRecentTransactions(),
                this.dataManager.requestTransactionDailyStats(),
                this.dataManager.requestNetworkUtilization(),
                this.dataManager.requestBlockSize(),
                this.dataManager.requestBlockRewards(),
                this.dataManager.requestAverageBlockTime(),
                this.dataManager.requestAverageKeyBlockTime()
            ]);
            
            // Subscribe to blockchain updates
            this.subscribeToBlockchainUpdates();
        } catch (error) {
            console.error("Error loading initial data:", error);
            this.handleError("Failed to load blockchain data. Please try again later.");
        } finally {
            this.ui.showLoading(false);
        }
    }
    
    /**
     * Controller thread function - runs periodically to update data
     */
    controllerThreadF() {
        if (this.mControlerExecuting || !this.initialized)
            return false;

        this.mControlerExecuting = true; // mutex protection
        
        try {
            // Update blockchain status and other key metrics
            Promise.all([
                this.dataManager.requestLiveness(),
                this.dataManager.requestHeight(),
                this.dataManager.requestUSDTPrice(),
                this.dataManager.requestBlockchainStatus()
            ]).catch(error => {
                console.error("Error updating status:", error);
            });

            // Check if we need to update other data based on active section
            const activeSection = this.getActiveSection();
            if (activeSection === 'dashboard') {
                // Update dashboard data
                Promise.all([
                    this.dataManager.requestRecentBlocks(10),
                    this.dataManager.requestRecentTransactions(10),
                    this.dataManager.requestNetworkUtilization(),
                    this.dataManager.requestBlockSize(),
                    this.dataManager.requestBlockRewards(),
                    this.dataManager.requestAverageBlockTime()
                ]).catch(error => {
                    console.error("Error updating dashboard:", error);
                });
            }
        } finally {
            this.mControlerExecuting = false;
        }
        
        return true;
    }
    
    /**
     * Get the currently active section
     * @returns {string} Active section ID
     */
    getActiveSection() {
        const activeNavItem = this.getWindowDocument().querySelector('.nav-item.active');
        return activeNavItem ? activeNavItem.getAttribute('data-section') : 'dashboard';
    }
    
    /**
     * Perform search based on user input
     * @param {string} query - Search query
     */
    async performSearch(query) {
        if (!query || query.trim() === '') return;
        
        this.ui.showLoading(true);
        const cleanQuery = query.trim();
        this.lastSearchQuery = cleanQuery;
        
        try {
            // Get CTools instance for validation
            const tools = CTools.getInstance();
            
            // Create a proper search filter based on current configuration
            let searchFilter = null;
            
            if (this.searchConfigPanel) {
                // Create a search filter from the current configuration
                searchFilter = this.searchConfigPanel.createSearchFilter(CSearchFilter);
            }
            
            // Determine search strategy based on query format
            
            // Check if it's a domain (containing a dot and passing validation) - base-58 decodeding + checksum validation
            if (tools.isDomainIDValid && tools.isDomainIDValid(cleanQuery)) {
                try {
                    await this.searchDomain(cleanQuery);
                    return;
                } catch (domainError) {
                    console.log("Domain search failed, falling back to general search:", domainError);
                    // Fall through to general search
                }
            } 
            // Check if it could be a receipt ID
            else if (tools.isReceiptIDValid) {  
                const receiptCheck = tools.isReceiptIDValid(cleanQuery);  //- base-58 decodeding + checksum validation
                if (receiptCheck && receiptCheck.isValid) {
                    try {
                        // Try to use the decoded receipt ID as a transaction ID
                        const decodedStr = new TextDecoder().decode(receiptCheck.decoded);
                        await this.viewTransactionDetails(decodedStr);
                        return;
                    } catch (receiptError) {
                        console.log("Receipt ID didn't match a transaction, falling back to general search");
                        // Fall through to general search
                    }
                }
            }
            
            // Default: perform a general blockchain search
            await this.dataManager.searchBlockchain(cleanQuery, searchFilter);
            
        } catch (error) {
            console.error("Error performing search:", error);
            this.handleError(`Search failed: ${error.message || 'Unknown error'}`);
        } finally {
            this.ui.showLoading(false);
        }
    }
    
    /**
     * View details for a specific block
     * @param {string} blockId - Block ID
     */
    async viewBlockDetails(blockId) {
        this.ui.showLoading(true);
        
        try {
            const blockDetails = await this.dataManager.requestBlockDetails(blockId);
            this.ui.switchToSection('blocks');
            return blockDetails;
        } catch (error) {
            console.error("Error viewing block details:", error);
            this.handleError(`Failed to fetch block details: ${error.message}`);
            throw error; // Re-throw for chain handling
        } finally {
            this.ui.showLoading(false);
        }
    }
    
    /**
     * View details for a specific transaction
     * @param {string} transactionId - Transaction ID
     */
    async viewTransactionDetails(transactionId) {
        this.ui.showLoading(true);
        
        try {
            const txDetails = await this.dataManager.requestTransactionDetails(transactionId);
            this.ui.switchToSection('transactions');
            return txDetails;
        } catch (error) {
            console.error("Error viewing transaction details:", error);
            this.handleError(`Failed to fetch transaction details: ${error.message}`);
            throw error; // Re-throw for chain handling
        } finally {
            this.ui.showLoading(false);
        }
    }
    
    /**
     * Search for a specific domain
     * @param {string} domain - Domain address
     */
    async searchDomain(domain) {
        if (!domain || domain.trim() === '') return;
        
        this.ui.showLoading(true);
        
        try {
            // Get domain details
            const domainDetails = await this.dataManager.requestDomainDetails(domain);
            
            // Get domain transaction history
            const transactions = await this.dataManager.requestDomainHistory(domain);
            
            // Update UI
            this.ui.switchToSection('domains');
        } catch (error) {
            console.error("Error searching domain:", error);
            this.handleError(`Failed to find domain: ${domain}`);
        } finally {
            this.ui.showLoading(false);
        }
    }
    
    /**
     * Subscribe to blockchain updates
     */
    subscribeToBlockchainUpdates() {
        if (this.subscriptionActive) return;
        
        const success = this.dataManager.subscribeToBlockchainUpdates();
        
        if (success) {
            this.subscriptionActive = true;
            console.log("Subscribed to blockchain updates");
        } else {
            console.error("Failed to subscribe to blockchain updates");
        }
    }
    
    /**
     * Unsubscribe from blockchain updates
     */
    unsubscribeFromBlockchainUpdates() {
        if (!this.subscriptionActive) return;
        
        const success = this.dataManager.unsubscribeFromBlockchainUpdates();
        
        if (success) {
            this.subscriptionActive = false;
            console.log("Unsubscribed from blockchain updates");
        } else {
            console.error("Failed to unsubscribe from blockchain updates");
        }
    }
    
    /**
     * Handle section change
     * @param {string} sectionId - Section ID
     */
    onSectionChange(sectionId) {
        // Load section-specific data
        switch (sectionId) {
            case 'dashboard':
                // Dashboard data is loaded on init and by controller thread
                break;
            case 'blocks':
                this.dataManager.requestRecentBlocks(50).catch(error => {
                    console.error("Error loading blocks data:", error);
                });
                break;
            case 'transactions':
                this.dataManager.requestRecentTransactions(50).catch(error => {
                    console.error("Error loading transactions data:", error);
                });
                break;
            case 'domains':
                // Domain data is loaded on demand through search
                break;
            case 'statistics':
                this.dataManager.requestTransactionDailyStats(30).catch(error => {
                    console.error("Error loading statistics data:", error);
                });
                
                Promise.all([
                    this.dataManager.requestNetworkUtilization(),
                    this.dataManager.requestBlockSize(),
                    this.dataManager.requestBlockRewards(),
                    this.dataManager.requestAverageBlockTime(),
                    this.dataManager.requestAverageKeyBlockTime()
                ]).catch(error => {
                    console.error("Error loading statistics data:", error);
                });
                break;
        }
    }
    
    /**
     * Handle window resize event
     * @param {boolean} isFallbackEvent - Whether this is a fallback event
     */
    finishResize(isFallbackEvent) {
        super.finishResize(isFallbackEvent);
        
        // Get current client dimensions
        const height = this.getClientHeight();
        const width = this.getClientWidth();
        
        // Update UI based on new dimensions
        if (this.ui) {
            this.ui.handleResize(width, height);
        }
    }
    
    /**
     * Handle mouse resize end event
     * @param {Object} handle - Resize handle
     */
    stopResize(handle) {
        super.stopResize(handle);
        this.finishResize(false);
    }
    
    /**
     * Handle window scroll event
     * @param {Event} event - Scroll event
     */
    onScroll(event) {
        super.onScroll(event);
        // Handle scroll events if needed
    }
    
    /**
     * Handle window open event
     */
    open() {
        super.open();
        this.initialize();
    }
    
    /**
     * Handle window close event
     */
    closeWindow() {
        // Clean up resources
        if (this.mControler > 0) {
            CVMContext.getInstance().stopJSThread(this.mControler);
            this.mControler = 0;
        }
        
        // Unsubscribe from blockchain updates
        if (this.subscriptionActive) {
            this.unsubscribeFromBlockchainUpdates();
        }
        
        // Clean up data manager
        if (this.dataManager) {
            this.dataManager.removeEventListeners();
        }
        
        // Clean up search config panel
        if (this.searchConfigPanel) {
            // No specific cleanup needed for now
        }
        
        // Clean up UI
        if (this.ui) {
            this.ui.cleanup();
        }
        
        super.closeWindow();
    }
    
    /**
     * User provided response to a data query
     * @param {Object} e - Response event
     */
    userResponseCallback(e) {
        console.log('User answered:', e.answer);
    }
}

/**
 * Data Manager - Handles API requests and data processing
 */
class BlockchainDataManager {
    /**
     * @param {Object} enums Blockchain enums
     * @param {Function} onUpdateCallback Callback for data updates
     * @param {Function} onErrorCallback Callback for errors
     */
    constructor(enums, onUpdateCallback, onErrorCallback) {
        this.enums = enums;
        this.onUpdate = onUpdateCallback;
        this.onError = onErrorCallback;
        this.vmContext = CVMContext.getInstance();
        this.pendingRequests = {};
        this.requestIDs = {};
        
        // Cache for blockchain data
        this.cache = {
            recentBlocks: [],
            recentTransactions: [],
            transactionDailyStats: [],
            blockchainStatus: null,
            networkUtilization: 0,
            blockSize: 0,
            blockRewards: 0,
            avgBlockTime: 0,
            avgKeyBlockTime: 0,
            currentHeight: 0,
            keyHeight: 0,
            usdtPrice: 0,
            liveness: true
        };
        
        // Bind methods to ensure correct 'this' context
        this.handleSearchResults = this.handleSearchResults.bind(this);
        this.handleBlockDetails = this.handleBlockDetails.bind(this);
        this.handleDomainDetails = this.handleDomainDetails.bind(this);
        this.handleTransactionDetails = this.handleTransactionDetails.bind(this);
        this.handleBlockchainStats = this.handleBlockchainStats.bind(this);
    }
    
    /**
     * Initialize event listeners for blockchain data
     * @param {string|number} windowID The window ID for event registration
     */
    initEventListeners(windowID) {
        this.windowID = windowID;
        
        // Register for blockchain-specific events
        this.vmContext.addNewSearchResultsListener(this.handleSearchResults, windowID);
        this.vmContext.addNewBlockDetailsListener(this.handleBlockDetails, windowID);
        this.vmContext.addNewDomainDetailsListener(this.handleDomainDetails, windowID);
        this.vmContext.addNewTransactionDetailsListener(this.handleTransactionDetails, windowID);
        this.vmContext.addBlockchainStatsListener(this.handleBlockchainStats, windowID);
    }
    
    /**
     * Remove all event listeners
     */
    removeEventListeners() {
        if (!this.windowID) return;
        
        try {
            const listeners = [
                'addNewSearchResultsListener',
                'addNewBlockDetailsListener',
                'addNewDomainDetailsListener',
                'addNewTransactionDetailsListener',
                'addBlockchainStatsListener'
            ];
            
            // For each listener type, check if a removal method exists before calling it
            listeners.forEach(listenerType => {
                const removeMethodName = listenerType.replace('add', 'remove');
                if (typeof this.vmContext[removeMethodName] === 'function') {
                    this.vmContext[removeMethodName](this.windowID);
                }
            });
        } catch (e) {
            console.warn("Error removing event listeners:", e);
        }
    }
    
    // Requests Management - BEGIN
    
    /**
     * Generate a unique request ID
     * @param {string} type Request type
     * @returns {number} Unique request ID
     */
    generateRequestID(type) {
        this.requestIDs[type] = (this.requestIDs[type] || 0) + 1;
        return Date.now() * 1000 + this.requestIDs[type]; // Ensure uniqueness
    }
    
    /**
     * Create a pending request and set up timeout handling
     * @param {number} requestID Request ID
     * @param {string} type Request type
     * @param {Object} additionalData Additional data to store with the request
     * @returns {Promise} Promise that resolves with the response or rejects on timeout
     */
    createPendingRequest(requestID, type, additionalData = {}) {
        return new Promise((resolve, reject) => {
            const request = {
                resolve,
                reject,
                type,
                ...additionalData,
                timestamp: Date.now()
            };
            
            this.pendingRequests[requestID] = request;
            
            // Set a timeout to reject the promise if no response is received
            const timeoutId = setTimeout(() => {
                if (this.pendingRequests[requestID]) {
                    delete this.pendingRequests[requestID];
                    const error = new Error(`Request timed out: ${type}`);
                    this.onError(error);
                    reject(error);
                }
            }, 30000); // 30 second timeout
            
            // Store the timeout ID so we can clear it if the request succeeds
            request.timeoutId = timeoutId;
        });
    }
    
    /**
     * Resolve a pending request
     * @param {number} requestID Request ID
     * @param {*} data Data to resolve the request with
     */
    resolvePendingRequest(requestID, data) {
        const request = this.pendingRequests[requestID];
        if (!request) return;
        
        clearTimeout(request.timeoutId);
        request.resolve(data);
        delete this.pendingRequests[requestID];
    }
    
    /**
     * Reject a pending request
     * @param {number} requestID Request ID
     * @param {Error} error Error to reject the request with
     */
    rejectPendingRequest(requestID, error) {
        const request = this.pendingRequests[requestID];
        if (!request) return;
        
        clearTimeout(request.timeoutId);
        request.reject(error);
        delete this.pendingRequests[requestID];
    }
    
    // Requests Management - END
    
    // API Request Methods
    
    /**
     * Request blockchain status
     * @returns {Promise<Object>} Blockchain status
     */
    async requestBlockchainStatus() {
        const reqID = this.generateRequestID('status');
        const success = this.vmContext.getBlockchainStatus(
            new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error('Failed to request blockchain status');
        }
        
        return this.createPendingRequest(reqID, this.enums.eRequestType.BLOCKCHAIN_STATUS);
    }
    
    /**
     * Request network liveness
     * @returns {Promise<boolean>} Network liveness state
     */
    async requestLiveness() {
        const reqID = this.generateRequestID('liveness');
        const success = this.vmContext.getLiveness(
            new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error('Failed to request blockchain liveness');
        }
        
        return this.createPendingRequest(reqID, this.enums.eRequestType.LIVENESS);
    }
    
    /**
     * Request USDT price
     * @returns {Promise<number>} USDT price
     */
    async requestUSDTPrice() {
        const reqID = this.generateRequestID('usdtPrice');
        const success = this.vmContext.getUSDTPrice(
            new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error('Failed to request USDT price');
        }
        
        return this.createPendingRequest(reqID, this.enums.eRequestType.USDT_PRICE);
    }
    
    /**
     * Request current blockchain height
     * @returns {Promise<Object>} Object containing regular and key heights
     */
    async requestHeight() {
        // Request regular height
        const reqID = this.generateRequestID('height');
        const success = this.vmContext.getHeight(
            false, new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error('Failed to request blockchain height');
        }
        
        // Request key height in parallel
        const keyReqID = this.generateRequestID('keyHeight');
        const keySuccess = this.vmContext.getHeight(
            true, new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, keyReqID
        );
        
        if (!keySuccess) {
            console.warn('Failed to request key height, proceeding with regular height only');
        }
        
        // Wait for both heights
        const [height, keyHeight] = await Promise.all([
            this.createPendingRequest(reqID, this.enums.eRequestType.HEIGHT),
            keySuccess ? 
                this.createPendingRequest(keyReqID, this.enums.eRequestType.KEY_HEIGHT).catch(() => 0) : 
                Promise.resolve(0)
        ]);
        
        this.cache.currentHeight = height;
        this.cache.keyHeight = keyHeight;
        
        return { height, keyHeight };
    }
    
    /**
     * Request recent blocks
     * @param {number} size Number of blocks to request
     * @returns {Promise<Array>} Array of recent blocks
     */
    async requestRecentBlocks(size = 10) {
        const reqID = this.generateRequestID('blocks');
        
        // Use enum value for timestamp descending sort
        // Note: API uses lowercase 'timestampDesc', not 'TIMESTAMP_DESC'
        const sortBlocksByTimestampDesc = this.enums.eSortBlocksBy.timestampDesc || 3; // Fallback to value 3 if enum not available
        
        const success = this.vmContext.getRecentBlocks(
            size, 1, sortBlocksByTimestampDesc, new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error('Failed to request recent blocks');
        }
        
        const blocks = await this.createPendingRequest(reqID, this.enums.eRequestType.RECENT_BLOCKS, { size });
        this.cache.recentBlocks = blocks;
        return blocks;
    }
    
    /**
     * Request recent transactions
     * @param {number} size Number of transactions to request
     * @returns {Promise<Array>} Array of recent transactions
     */
    async requestRecentTransactions(size = 10) {
        const reqID = this.generateRequestID('transactions');
        
        // Note: API expects includeMem parameter (boolean) before threadID
        const includeMem = false; // Default value 
        
        const success = this.vmContext.getRecentTransactions(
            size, 1, includeMem, new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error('Failed to request recent transactions');
        }
        
        const transactions = await this.createPendingRequest(reqID, this.enums.eRequestType.RECENT_TRANSACTIONS, { size });
        this.cache.recentTransactions = transactions;
        return transactions;
    }
    
    /**
     * Request transaction daily statistics
     * @param {number} days Number of days to request stats for
     * @returns {Promise<Array>} Array of daily statistics
     */
    async requestTransactionDailyStats(days = 14) {
        const reqID = this.generateRequestID('stats');
        
        // Note: API expects includeMem parameter
        const includeMem = false; // Default value
        
        const success = this.vmContext.getTransactionDailyStats(
            days, includeMem, new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error('Failed to request transaction daily stats');
        }
        
        const stats = await this.createPendingRequest(reqID, this.enums.eRequestType.TRANSACTION_DAILY_STATS, { days });
        this.cache.transactionDailyStats = stats;
        return stats;
    }
    
    /**
     * Request network utilization (24h)
     * @returns {Promise<number>} Network utilization percentage
     */
    async requestNetworkUtilization() {
        const reqID = this.generateRequestID('networkUtil');
        
        const success = this.vmContext.getNetworkUtilization24h(
            new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error('Failed to request network utilization');
        }
        
        const utilization = await this.createPendingRequest(reqID, this.enums.eRequestType.NETWORK_UTILIZATION);
        this.cache.networkUtilization = utilization;
        return utilization;
    }
    
    /**
     * Request average block size (24h)
     * @returns {Promise<number>} Average block size in bytes
     */
    async requestBlockSize() {
        const reqID = this.generateRequestID('blockSize');
        
        const success = this.vmContext.getBlockSize24h(
            new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error('Failed to request average block size');
        }
        
        const blockSize = await this.createPendingRequest(reqID, this.enums.eRequestType.BLOCK_SIZE);
        this.cache.blockSize = blockSize;
        return blockSize;
    }
    
    /**
     * Request average block rewards (24h)
     * @returns {Promise<number>} Average block rewards
     */
    async requestBlockRewards() {
        const reqID = this.generateRequestID('blockRewards');
        
        const success = this.vmContext.getBlockRewards24h(
            new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error('Failed to request block rewards');
        }
        
        const blockRewards = await this.createPendingRequest(reqID, this.enums.eRequestType.BLOCK_REWARDS);
        this.cache.blockRewards = blockRewards;
        return blockRewards;
    }
    
    /**
     * Request average block time (24h)
     * @returns {Promise<number>} Average block time in seconds
     */
    async requestAverageBlockTime() {
        const reqID = this.generateRequestID('blockTime');
        
        const success = this.vmContext.getAverageBlockTime24h(
            new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error('Failed to request average block time');
        }
        
        const blockTime = await this.createPendingRequest(reqID, this.enums.eRequestType.AVERAGE_BLOCK_TIME);
        this.cache.avgBlockTime = blockTime;
        return blockTime;
    }
    
    /**
     * Request average key block time (24h)
     * @returns {Promise<number>} Average key block time in seconds
     */
    async requestAverageKeyBlockTime() {
        const reqID = this.generateRequestID('keyBlockTime');
        
        const success = this.vmContext.getAverageKeyBlockTime24h(
            new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error('Failed to request average key block time');
        }
        
        const keyBlockTime = await this.createPendingRequest(reqID, this.enums.eRequestType.AVERAGE_KEY_BLOCK_TIME);
        this.cache.avgKeyBlockTime = keyBlockTime;
        return keyBlockTime;
    }
    
    /**
     * Request details for a specific block
     * @param {string} blockID Block ID
     * @returns {Promise<Object>} Block details
     */
    async requestBlockDetails(blockID) {
        const reqID = this.generateRequestID('blockDetails');
        
        // Note: API expects includeTX, includeMem, includeSec parameters
        const includeTX = true;
        const includeMem = false;
        const includeSec = false;
        
        const success = this.vmContext.getBlockDetails(
            blockID, includeTX, includeMem, includeSec, new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error(`Failed to request details for block: ${blockID}`);
        }
        
        return this.createPendingRequest(reqID, this.enums.eRequestType.BLOCK_DETAILS, { blockID });
    }
    
    /**
     * Request details for a specific transaction
     * @param {string} transactionID Transaction ID
     * @returns {Promise<Object>} Transaction details
     */
    async requestTransactionDetails(transactionID) {
        const reqID = this.generateRequestID('transactionDetails');
        
        const success = this.vmContext.getTransactionDetails(
            transactionID, new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error(`Failed to request details for transaction: ${transactionID}`);
        }
        
        return this.createPendingRequest(reqID, this.enums.eRequestType.TRANSACTION_DETAILS, { transactionID });
    }
    
    /**
     * Request details for a domain
     * @param {string} address Domain address
     * @returns {Promise<Object>} Domain details
     */
    async requestDomainDetails(address) {
        const reqID = this.generateRequestID('domainDetails');
        
        // Note: API expects perspective and includeSec parameters
        const perspective = "";
        const includeSec = false;
        
        const success = this.vmContext.getDomainDetails(
            address, perspective, includeSec, new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error(`Failed to request details for domain: ${address}`);
        }
        
        return this.createPendingRequest(reqID, this.enums.eRequestType.DOMAIN_DETAILS, { address });
    }
    
    /**
     * Request transaction history for a domain
     * @param {string} address Domain address
     * @param {number} size Number of transactions to request
     * @returns {Promise<Array>} Domain transaction history
     */
    async requestDomainHistory(address, size = 20) {
        const reqID = this.generateRequestID('domainHistory');
        
        // API Parameters:
        // address: Domain address
        // size: Number of transactions per page
        // page: Page number
        // eSortTransactionsByVal: Sort order
        // stateID: State ID for restoring iterators
        
        // Use enum value for timestamp descending sort or fallback to 3
        const sortByTimestampDesc = this.enums.eSortTransactionsBy.timestampDesc || 3; 
        const page = 1;
        const stateID = "";
        
        const success = this.vmContext.getDomainHistory(
            address, size, page, sortByTimestampDesc, stateID, new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error(`Failed to request history for domain: ${address}`);
        }
        
        return this.createPendingRequest(reqID, this.enums.eRequestType.DOMAIN_HISTORY, { address, size });
    }
    
    /**
     * Search the blockchain with a query string
     * @param {string} query Query string
     * @param {CSearchFilter|null} searchFilter Optional search filter
     * @returns {Promise<Array>} Search results
     */
    async searchBlockchain(query, searchFilter = null) {
        const reqID = this.generateRequestID('search');
        
        // If searchFilter wasn't provided, try to create one
        if (!searchFilter) {
            try {
                searchFilter = new CSearchFilter();
                // Set default search flags
                searchFilter.setBlockSearch(true);
                searchFilter.setTransactionSearch(true);
                searchFilter.setDomainSearch(true);
            } catch (e) {
                console.warn("CSearchFilter not available, using null flags");
                searchFilter = null;
            }
        }
        
        // API Parameters:
        // query: Search query
        // size: Number of results
        // page: Page number
        // flags: Search filter
        // threadID: Thread ID
        // processHandle: Process handle
        // mode: Execution mode
        // reqID: Request ID
        
        const size = 10;
        const page = 1;
        
        const success = this.vmContext.searchBlockchain(
            query, size, page, searchFilter, new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, reqID
        );
        
        if (!success) {
            throw new Error(`Failed to search for "${query}"`);
        }
        
        this.lastSearchQuery = query;
        return this.createPendingRequest(reqID, this.enums.eRequestType.SEARCH, { query });
    }
    
    /**
     * Subscribe to blockchain updates
     * @returns {boolean} Success status
     */
    subscribeToBlockchainUpdates() {
        // API parameters:
        // threadID: Thread ID
        // processHandle: Process handle
        // mode: Execution mode
        // reqID: Request ID
        
        const success = this.vmContext.subscribeToBlockchainUpdates(
            new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, 0
        );
        
        return success;
    }
    
    /**
     * Unsubscribe from blockchain updates
     * @returns {boolean} Success status
     */
    unsubscribeFromBlockchainUpdates() {
        // API parameters:
        // threadID: Thread ID
        // processHandle: Process handle
        // mode: Execution mode
        // reqID: Request ID
        
        const success = this.vmContext.unsubscribeFromBlockchainUpdates(
            new ArrayBuffer(), null, this.enums.eVMMetaCodeExecutionMode.RAW, 0
        );
        
        return success;
    }
    
    // Event handlers for blockchain data
    
    /**
     * Handler for search results events
     * @param {Object} arg Event argument containing search results
     */
    handleSearchResults(arg) {
        if (!arg || !arg.results) return;
        
        const { results, reqID } = arg;
        const pendingRequest = this.pendingRequests[reqID];
        
        if (!pendingRequest) return;
        
        try {
            // Verify the object is a CSearchResults instance
            if (!(results instanceof CSearchResults)) {
                throw new Error("Received object is not a CSearchResults instance");
            }
            
            // Process the results based on the request type
            let processedResults;
            
            switch (pendingRequest.type) {
                case this.enums.eRequestType.SEARCH:
                    processedResults = this.vmContext.processSearchResults(results);
                    break;
                    
                case this.enums.eRequestType.RECENT_BLOCKS:
                    processedResults = this.vmContext.processBlocksFromSearchResults(results);
                    this.cache.recentBlocks = processedResults;
                    break;
                    
                case this.enums.eRequestType.RECENT_TRANSACTIONS:
                    processedResults = this.vmContext.processTransactionsFromSearchResults(results);
                    this.cache.recentTransactions = processedResults;
                    break;
                    
                case this.enums.eRequestType.DOMAIN_HISTORY:
                    processedResults = this.vmContext.processDomainHistoryFromSearchResults(results, pendingRequest.address);
                    break;
                    
                default:
                    console.warn(`Unknown request type for search results: ${pendingRequest.type}`);
                    processedResults = this.vmContext.processSearchResults(results);
            }
            
            // Update the relevant UI sections through the callback
            if (this.onUpdate) {
                this.onUpdate(pendingRequest.type, processedResults);
            }
            
            // Resolve the pending request
            this.resolvePendingRequest(reqID, processedResults);
        } catch (error) {
            console.error(`Error processing search results:`, error);
            
            // Reject the pending request
            this.rejectPendingRequest(reqID, error);
            
            // Notify about the error
            if (this.onError) {
                this.onError(error);
            }
        }
    }
    
    /**
     * Handler for block details events
     * @param {Object} arg Event argument containing block details
     */
    handleBlockDetails(arg) {
        if (!arg || !arg.blockDesc) return;
        
        const { blockDesc, reqID } = arg;
        const pendingRequest = this.pendingRequests[reqID];
        
        if (!pendingRequest) return;
        
        try {
            // Verify the object is a CBlockDesc instance
            if (!(blockDesc instanceof CBlockDesc)) {
                throw new Error("Received object is not a CBlockDesc instance");
            }
            
            // Process the CBlockDesc object
            const processedBlock = this.vmContext.processBlockData(blockDesc);
            
            // Update the UI through the callback
            if (this.onUpdate) {
                this.onUpdate(pendingRequest.type, processedBlock);
            }
            
            // Resolve the pending request
            this.resolvePendingRequest(reqID, processedBlock);
        } catch (error) {
            console.error('Error processing block details:', error);
            
            // Reject the pending request
            this.rejectPendingRequest(reqID, error);
            
            // Notify about the error
            if (this.onError) {
                this.onError(error);
            }
        }
    }
    
    /**
     * Handler for domain details events
     * @param {Object} arg Event argument containing domain details
     */
    handleDomainDetails(arg) {
        if (!arg || !arg.domainDesc) return;
        
        const { domainDesc, reqID } = arg;
        const pendingRequest = this.pendingRequests[reqID];
        
        if (!pendingRequest) return;
        
        try {
            // Verify the object is a CDomainDesc instance
            if (!(domainDesc instanceof CDomainDesc)) {
                throw new Error("Received object is not a CDomainDesc instance");
            }
            
            // Process the CDomainDesc object
            const processedDomain = this.vmContext.processDomainData(domainDesc);
            
            // Update the UI through the callback
            if (this.onUpdate) {
                this.onUpdate(pendingRequest.type, processedDomain);
            }
            
            // Resolve the pending request
            this.resolvePendingRequest(reqID, processedDomain);
        } catch (error) {
            console.error('Error processing domain details:', error);
            
            // Reject the pending request
            this.rejectPendingRequest(reqID, error);
            
            // Notify about the error
            if (this.onError) {
                this.onError(error);
            }
        }
    }
    
    /**
     * Handler for transaction details events
     * @param {Object} arg Event argument containing transaction details
     */
    handleTransactionDetails(arg) {
        if (!arg || !arg.transactionDesc) return;
        
        const { transactionDesc, reqID } = arg;
        const pendingRequest = this.pendingRequests[reqID];
        
        if (!pendingRequest) return;
        
        try {
            // Verify the object is a CTransactionDesc instance
            if (!(transactionDesc instanceof CTransactionDesc)) {
                throw new Error("Received object is not a CTransactionDesc instance");
            }
            
            // Process the CTransactionDesc object
            const processedTx = this.vmContext.processTransactionData(transactionDesc);
            
            // Update the UI through the callback
            if (this.onUpdate) {
                this.onUpdate(pendingRequest.type, processedTx);
            }
            
            // Resolve the pending request
            this.resolvePendingRequest(reqID, processedTx);
        } catch (error) {
            console.error('Error processing transaction details:', error);
            
            // Reject the pending request
            this.rejectPendingRequest(reqID, error);
            
            // Notify about the error
            if (this.onError) {
                this.onError(error);
            }
        }
    }
    
    /**
     * Handler for blockchain statistics events
     * @param {Object} arg Event argument containing blockchain statistics
     */
    handleBlockchainStats(arg) {
        if (!arg || !arg.data) return;
        
        const { type, data, reqID } = arg;
        const pendingRequest = this.pendingRequests[reqID];
        
        try {
            // Process the statistics based on the type
            if (pendingRequest) {
                const requestType = pendingRequest.type;
                let processedData = data;
                
                switch (requestType) {
                    case this.enums.eRequestType.BLOCKCHAIN_STATUS:
                        this.cache.blockchainStatus = data;
                        break;
                        
                    case this.enums.eRequestType.NETWORK_UTILIZATION:
                        processedData = data.utilization || data;
                        this.cache.networkUtilization = processedData;
                        break;
                        
                    case this.enums.eRequestType.BLOCK_SIZE:
                        processedData = data.averageSize || data;
                        this.cache.blockSize = processedData;
                        break;
                        
                    case this.enums.eRequestType.BLOCK_REWARDS:
                        processedData = data.averageReward || data;
                        this.cache.blockRewards = processedData;
                        break;
                        
                    case this.enums.eRequestType.AVERAGE_BLOCK_TIME:
                        processedData = data.averageTime || data;
                        this.cache.avgBlockTime = processedData;
                        break;
                        
                    case this.enums.eRequestType.AVERAGE_KEY_BLOCK_TIME:
                        processedData = data.averageTime || data;
                        this.cache.avgKeyBlockTime = processedData;
                        break;
                        
                    case this.enums.eRequestType.TRANSACTION_DAILY_STATS:
                        processedData = data.dailyStats || data;
                        this.cache.transactionDailyStats = processedData;
                        break;
                        
                    case this.enums.eRequestType.LIVENESS:
                        processedData = typeof data.state !== 'undefined' ? data.state : data;
                        this.cache.liveness = processedData;
                        break;
                        
                    case this.enums.eRequestType.USDT_PRICE:
                        processedData = data.price || data;
                        this.cache.usdtPrice = processedData;
                        break;
                        
                    case this.enums.eRequestType.HEIGHT:
                        processedData = data.height || data;
                        this.cache.currentHeight = processedData;
                        break;
                        
                    case this.enums.eRequestType.KEY_HEIGHT:
                        processedData = data.height || data;
                        this.cache.keyHeight = processedData;
                        break;
                }
                
                // Update the UI through the callback
                if (this.onUpdate) {
                    this.onUpdate(requestType, processedData);
                }
                
                // Resolve the pending request
                this.resolvePendingRequest(reqID, processedData);
            } else {
                // This might be a subscription update, so update our cache based on the stats type
                if (type === 'blockchainStatus') {
                    this.cache.blockchainStatus = data;
                    if (this.onUpdate) this.onUpdate(this.enums.eRequestType.BLOCKCHAIN_STATUS, data);
                } else if (type === 'blockchainHeight') {
                    if (data.keyHeight) {
                        this.cache.keyHeight = data.keyHeight;
                        if (this.onUpdate) this.onUpdate(this.enums.eRequestType.KEY_HEIGHT, data.keyHeight);
                    }
                    if (data.height) {
                        this.cache.currentHeight = data.height;
                        if (this.onUpdate) this.onUpdate(this.enums.eRequestType.HEIGHT, data.height);
                    }
                } else if (type === 'usdtPrice') {
                    this.cache.usdtPrice = data.price || data;
                    if (this.onUpdate) this.onUpdate(this.enums.eRequestType.USDT_PRICE, data.price || data);
                } else if (type === 'livenessInfo') {
                    this.cache.liveness = data.state || data;
                    if (this.onUpdate) this.onUpdate(this.enums.eRequestType.LIVENESS, data.state || data);
                } else if (type === 'marketDepth' || type === 'networkUtilization') {
                    this.cache.networkUtilization = data.utilization || data;
                    if (this.onUpdate) this.onUpdate(this.enums.eRequestType.NETWORK_UTILIZATION, data.utilization || data);
                } else if (type === 'transactionDailyStats') {
                    this.cache.transactionDailyStats = data;
                    if (this.onUpdate) this.onUpdate(this.enums.eRequestType.TRANSACTION_DAILY_STATS, data);
                }
            }
        } catch (error) {
            console.error('Error processing blockchain stats:', error);
            
            // Reject the pending request if there is one
            if (pendingRequest) {
                this.rejectPendingRequest(reqID, error);
            }
            
            // Notify about the error
            if (this.onError) {
                this.onError(error);
            }
        }
    }
}

/**
 * UI Manager - Handles UI components and updates
 * Responsible for handling the user interface aspects of the blockchain explorer
 */
class BlockchainExplorerUI {
    /**
     * @param {Object} elements UI element references
     * @param {Object} enums Blockchain enums
     */
    constructor(elements, enums) {
        this.elements = elements;
        this.enums = enums;
        
        // Table instances
        this.tables = {};
        
        // Chart instances
        this.charts = {};
        
        // Current domain being viewed
        this.currentDomain = "";
        
        // Initialization state
        this.initialized = false;
    }
    
    /**
     * Initialize UI tables
     */
    initTables() {
        // Recent blocks table
        this.tables.recentBlocks = new Tabulator("#recent-blocks-table", {
            height: "300px",
            layout: "fitColumns",
            placeholder: "No Block Data Available",
            columns: [
                {title: "Height", field: "height", sorter: "number", headerSortStartingDir: "desc", width: 100},
                {title: "Block ID", field: "blockID", formatter: this.hashFormatter},
                {title: "Miner", field: "minerID", formatter: this.hashFormatter},
                {title: "Time", field: "solvedAt", formatter: this.timestampFormatter, width: 180},
                {title: "Transactions", field: "transactionsCount", sorter: "number", width: 120},
                {title: "Size", field: "size", formatter: this.sizeFormatter, width: 100}
            ],
            rowClick: (e, row) => {
                this.onViewBlockDetails(row.getData().blockID);
            }
        });
        
        // Recent transactions table
        this.tables.recentTransactions = new Tabulator("#recent-transactions-table", {
            height: "300px",
            layout: "fitColumns",
            placeholder: "No Transaction Data Available",
            columns: [
                {title: "Transaction ID", field: "verifiableID", formatter: this.hashFormatter},
                {title: "Status", field: "status", formatter: this.statusFormatter, width: 120},
                {title: "From", field: "sender", formatter: this.hashFormatter},
                {title: "To", field: "receiver", formatter: this.hashFormatter},
                {title: "Value", field: "value", formatter: this.valueFormatter, width: 120},
                {title: "Time", field: "time", formatter: this.timestampFormatter, width: 180}
            ],
            rowClick: (e, row) => {
                this.onViewTransactionDetails(row.getData().verifiableID);
            }
        });
        
        // Blocks table (full page)
        this.tables.blocks = new Tabulator("#blocks-table", {
            height: "600px",
            layout: "fitColumns",
            pagination: true,
            paginationSize: 20,
            placeholder: "No Block Data Available",
            columns: [
                {title: "Height", field: "height", sorter: "number", headerSortStartingDir: "desc", width: 100},
                {title: "Block ID", field: "blockID", formatter: this.hashFormatter},
                {title: "Miner", field: "minerID", formatter: this.hashFormatter},
                {title: "Time", field: "solvedAt", formatter: this.timestampFormatter, width: 180},
                {title: "Transactions", field: "transactionsCount", sorter: "number", width: 120},
                {title: "Difficulty", field: "difficulty", width: 120},
                {title: "Size", field: "size", formatter: this.sizeFormatter, width: 100}
            ],
            rowClick: (e, row) => {
                this.onViewBlockDetails(row.getData().blockID);
            }
        });
        
        // Transactions table (full page)
        this.tables.transactions = new Tabulator("#transactions-table", {
            height: "600px",
            layout: "fitColumns",
            pagination: true,
            paginationSize: 20,
            placeholder: "No Transaction Data Available",
            columns: [
                {title: "Transaction ID", field: "verifiableID", formatter: this.hashFormatter},
                {title: "Status", field: "status", formatter: this.statusFormatter, width: 120},
                {title: "Block", field: "height", sorter: "number", width: 100},
                {title: "From", field: "sender", formatter: this.hashFormatter},
                {title: "To", field: "receiver", formatter: this.hashFormatter},
                {title: "Value", field: "value", formatter: this.valueFormatter, width: 120},
                {title: "Time", field: "time", formatter: this.timestampFormatter, width: 180}
            ],
            rowClick: (e, row) => {
                this.onViewTransactionDetails(row.getData().verifiableID);
            }
        });
        
        // Domain history table
        this.tables.domainHistory = new Tabulator("#domain-history-table", {
            height: "400px",
            layout: "fitColumns",
            placeholder: "No Domain History Available",
            columns: [
                {title: "Transaction ID", field: "verifiableID", formatter: this.hashFormatter},
                {title: "Type", field: "type", width: 100},
                {title: "Value", field: "value", formatter: this.valueFormatter, width: 120},
                {title: "From/To", field: "counterparty", formatter: this.hashFormatter},
                {title: "Time", field: "time", formatter: this.timestampFormatter, width: 180}
            ],
            rowClick: (e, row) => {
                this.onViewTransactionDetails(row.getData().verifiableID);
            }
        });
    }
    
    /**
     * Register event callbacks
     * @param {Object} callbacks Object with callback functions
     */
    registerCallbacks(callbacks) {
        this.callbacks = callbacks;
    }
    
    /**
     * Initialize UI components
     */
    initialize() {
        if (this.initialized) return;
        
        // Initialize tables
        this.initTables();
        
        // Set up navigation events
        if (this.elements.navItems) {
            this.elements.navItems.forEach(item => {
                item.addEventListener('click', () => {
                    this.switchToSection(item.getAttribute('data-section'));
                    
                    if (this.callbacks && this.callbacks.onSectionChange) {
                        this.callbacks.onSectionChange(item.getAttribute('data-section'));
                    }
                });
            });
        }
        
        // Set up search functionality
        if (this.elements.searchButton && this.elements.searchInput) {
            this.elements.searchButton.addEventListener('click', () => {
                const query = this.elements.searchInput.value;
                if (this.callbacks && this.callbacks.onSearch) {
                    this.callbacks.onSearch(query);
                }
            });
            
            this.elements.searchInput.addEventListener('keypress', (e) => {
                if (e.key === 'Enter') {
                    const query = this.elements.searchInput.value;
                    if (this.callbacks && this.callbacks.onSearch) {
                        this.callbacks.onSearch(query);
                    }
                }
            });
        }
        
        // Set up domain search functionality
        if (this.elements.domainSearchButton && this.elements.domainSearchInput) {
            this.elements.domainSearchButton.addEventListener('click', () => {
                const domain = this.elements.domainSearchInput.value;
                if (this.callbacks && this.callbacks.onDomainSearch) {
                    this.callbacks.onDomainSearch(domain);
                }
            });
            
            this.elements.domainSearchInput.addEventListener('keypress', (e) => {
                if (e.key === 'Enter') {
                    const domain = this.elements.domainSearchInput.value;
                    if (this.callbacks && this.callbacks.onDomainSearch) {
                        this.callbacks.onDomainSearch(domain);
                    }
                }
            });
        }
        
        this.initialized = true;
    }
    
    /**
     * Show or hide loading overlay
     * @param {boolean} show Whether to show loading
     */
    showLoading(show) {
        if (this.elements && this.elements.loadingOverlay) {
            this.elements.loadingOverlay.style.display = show ? 'flex' : 'none';
        }
    }
    
    /**
     * Display error message
     * @param {string} message Error message
     */
    showError(message) {
        if (this.callbacks && this.callbacks.onError) {
            this.callbacks.onError(message);
        }
    }
    
    /**
     * Switch to a specific section
     * @param {string} sectionId Section ID to switch to
     */
    switchToSection(sectionId) {
        // Remove active class from all nav items and content sections
        if (this.elements.navItems) {
            this.elements.navItems.forEach(navItem => navItem.classList.remove('active'));
        }
        
        if (this.elements.contentSections) {
            this.elements.contentSections.forEach(section => section.classList.remove('active'));
        }
        
        // Add active class to specified nav item and content section
        const navItem = document.querySelector(`.nav-item[data-section="${sectionId}"]`);
        if (navItem) {
            navItem.classList.add('active');
            const sectionElement = document.getElementById(`${sectionId}-section`);
            if (sectionElement) {
                sectionElement.classList.add('active');
            }
        }
    }
    
    /**
     * Update status display with latest values
     * @param {Object} data Status data
     */
    updateStatusDisplay(data) {
        if (!this.elements) return;
        
        // Update blockchain status
        if (data.blockchainStatus && this.elements.blockchainStatus) {
            const status = data.blockchainStatus.status || 'Unknown';
            this.elements.blockchainStatus.textContent = `Network: ${status}`;
        }
        
        // Update height
        if (data.height !== undefined && this.elements.blockchainHeight) {
            this.elements.blockchainHeight.textContent = data.height.toString();
        }
        
        // Update USDT price
        if (data.usdtPrice !== undefined && this.elements.usdtPrice) {
            this.elements.usdtPrice.textContent = data.usdtPrice.toFixed(2);
        }
        
        // Update liveness indicator
        if (data.liveness !== undefined && this.elements.livenessIndicator) {
            if (data.liveness) {
                this.elements.livenessIndicator.classList.remove('status-offline');
                this.elements.livenessIndicator.classList.add('status-live');
            } else {
                this.elements.livenessIndicator.classList.remove('status-live');
                this.elements.livenessIndicator.classList.add('status-offline');
            }
        }
    }
    
    /**
     * Update dashboard display with latest values
     * @param {Object} data Dashboard data
     */
    updateDashboardDisplay(data) {
        if (!this.elements) return;
        
        // Update network stats
        if (data.networkUtilization !== undefined && this.elements.networkUtilization) {
            this.elements.networkUtilization.textContent = `${data.networkUtilization.toFixed(2)}%`;
        }
        
        if (data.blockSize !== undefined && this.elements.avgBlockSize) {
            this.elements.avgBlockSize.textContent = this.formatSize(data.blockSize);
        }
        
        if (data.blockRewards !== undefined && this.elements.blockRewards) {
            const tools = CTools.getInstance();
            if (tools && typeof tools.formatGNCValue === 'function') {
                this.elements.blockRewards.textContent = tools.formatGNCValue(data.blockRewards);
            } else {
                this.elements.blockRewards.textContent = `${data.blockRewards.toFixed(2)} GNC`;
            }
        }
        
        if (data.avgBlockTime !== undefined && this.elements.avgBlockTime) {
            this.elements.avgBlockTime.textContent = `${data.avgBlockTime.toFixed(2)} sec`;
        }
    }
    
    /**
     * Update statistics display with latest values
     * @param {Object} data Statistics data
     */
    updateStatisticsDisplay(data) {
        if (!this.elements) return;
        
        // Update 24h stats
        if (data.avgKeyBlockTime !== undefined && this.elements.keyBlockTime) {
            this.elements.keyBlockTime.textContent = `${data.avgKeyBlockTime.toFixed(2)} sec`;
        }
        
        if (data.networkUtilization !== undefined && this.elements.networkUtil24h) {
            this.elements.networkUtil24h.textContent = `${data.networkUtilization.toFixed(2)}%`;
        }
        
        if (data.blockRewards !== undefined && this.elements.blockRewards24h) {
            const tools = CTools.getInstance();
            if (tools && typeof tools.formatGNCValue === 'function') {
                this.elements.blockRewards24h.textContent = tools.formatGNCValue(data.blockRewards);
            } else {
                this.elements.blockRewards24h.textContent = `${data.blockRewards.toFixed(2)} GNC`;
            }
        }
        
        if (data.blockSize !== undefined && this.elements.blockSize24h) {
            this.elements.blockSize24h.textContent = this.formatSize(data.blockSize);
        }
    }
    
    /**
     * Update transactions chart with latest data
     * @param {Array} data Transaction daily stats
     */
    updateTransactionsChart(data) {
        const chart = document.getElementById('transactions-chart');
        if (!chart || !data || data.length === 0) return;
        
        // Process data for chart
        const dates = data.map(stat => new Date(stat.date * 1000).toLocaleDateString());
        const counts = data.map(stat => stat.count || 0);
        const volumes = data.map(stat => stat.volume || 0);
        
        // Create dual axis chart
        const chartData = [{
            x: dates,
            y: counts,
            type: 'scatter',
            mode: 'lines+markers',
            name: 'Transaction Count',
            marker: {color: 'var(--neon-purple)'},
            line: {color: 'var(--neon-purple)', width: 2}
        }, {
            x: dates,
            y: volumes,
            type: 'scatter',
            mode: 'lines+markers',
            name: 'Volume (GNC)',
            yaxis: 'y2',
            marker: {color: 'var(--neon-cyan)'},
            line: {color: 'var(--neon-cyan)', width: 2}
        }];
        
        const layout = {
            title: 'Daily Transaction Activity',
            paper_bgcolor: 'rgba(0,0,0,0)',
            plot_bgcolor: 'rgba(0,0,0,0)',
            font: {
                color: 'var(--text-primary)'
            },
            xaxis: {
                gridcolor: 'rgba(61, 110, 255, 0.1)'
            },
            yaxis: {
                title: 'Transaction Count',
                titlefont: {color: 'var(--neon-purple)'},
                tickfont: {color: 'var(--neon-purple)'},
                gridcolor: 'rgba(61, 110, 255, 0.1)'
            },
            yaxis2: {
                title: 'Volume (GNC)',
                titlefont: {color: 'var(--neon-cyan)'},
                tickfont: {color: 'var(--neon-cyan)'},
                overlaying: 'y',
                side: 'right',
                gridcolor: 'rgba(0,0,0,0)'
            },
            legend: {
                orientation: 'h',
                y: -0.2
            },
            margin: {
                l: 60,
                r: 60,
                t: 30,
                b: 80
            }
        };
        
        const config = {
            responsive: true,
            displayModeBar: false
        };
        
        try {
            Plotly.newPlot(chart, chartData, layout, config);
            this.charts.transactionsDaily = chart;
        } catch (error) {
            console.error("Error creating transactions chart:", error);
        }
    }
    
    /**
     * Update daily stats chart with latest data
     * @param {Array} data Transaction daily stats
     */
    updateDailyStatsChart(data) {
        const chart = document.getElementById('daily-stats-chart');
        if (!chart || !data || data.length === 0) return;
        
        // Use the transaction daily stats data for this chart
        const dates = data.map(stat => new Date(stat.date * 1000).toLocaleDateString());
        const fees = data.map(stat => stat.fees || 0);
        const blockRewards = data.map(stat => stat.rewards || 0);
        
        const chartData = [{
            x: dates,
            y: fees,
            type: 'bar',
            name: 'Transaction Fees',
            marker: {color: 'var(--neon-blue)'}
        }, {
            x: dates,
            y: blockRewards,
            type: 'bar',
            name: 'Block Rewards',
            marker: {color: 'var(--neon-green)'}
        }];
        
        const layout = {
            title: 'Daily Rewards & Fees (GNC)',
            barmode: 'stack',
            paper_bgcolor: 'rgba(0,0,0,0)',
            plot_bgcolor: 'rgba(0,0,0,0)',
            font: {
                color: 'var(--text-primary)'
            },
            xaxis: {
                gridcolor: 'rgba(61, 110, 255, 0.1)'
            },
            yaxis: {
                gridcolor: 'rgba(61, 110, 255, 0.1)'
            },
            legend: {
                orientation: 'h',
                y: -0.2
            },
            margin: {
                l: 60,
                r: 40,
                t: 30,
                b: 80
            }
        };
        
        const config = {
            responsive: true,
            displayModeBar: false
        };
        
        try {
            Plotly.newPlot(chart, chartData, layout, config);
            this.charts.dailyStats = chart;
        } catch (error) {
            console.error("Error creating daily stats chart:", error);
        }
    }
    
    /**
     * Format size in bytes to human-readable format
     * @param {number} bytes Size in bytes
     * @returns {string} Formatted size
     */
    formatSize(bytes) {
        if (bytes < 1024) return `${bytes} B`;
        else if (bytes < 1048576) return `${(bytes / 1024).toFixed(2)} KB`;
        else return `${(bytes / 1048576).toFixed(2)} MB`;
    }
    
    /**
     * Display block details in the UI
     * @param {Object} block Block data
     */
    displayBlockDetails(block) {
        const blockDetailsElement = document.getElementById('block-details');
        if (!blockDetailsElement) return;
        
        if (!block) {
            blockDetailsElement.innerHTML = '<p>No block details available</p>';
            return;
        }
        
        // Format the block data for display
        const html = `
            <div class="detail-view fade-in">
                <div class="detail-header">
                    <div class="detail-type">Block Details</div>
                    <div>Height: ${block.height}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Block ID</div>
                    <div class="detail-value hash-value">${block.blockID}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Timestamp</div>
                    <div class="detail-value">${new Date(block.solvedAt * 1000).toLocaleString()}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Miner</div>
                    <div class="detail-value hash-value">${block.minerID}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Parent Block</div>
                    <div class="detail-value hash-value">${block.parentID}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Key Height</div>
                    <div class="detail-value">${block.keyHeight}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Type</div>
                    <div class="detail-value">${block.type === 1 ? 'Key Block' : 'Data Block'}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Difficulty</div>
                    <div class="detail-value">${block.difficulty}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Total Difficulty</div>
                    <div class="detail-value">${block.totalDifficulty}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Nonce</div>
                    <div class="detail-value">${block.nonce}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Block Reward</div>
                    <div class="detail-value">${
                        block.blockRewardTxt || 
                        (() => {
                            const tools = CTools.getInstance();
                            return tools && typeof tools.formatGNCValue === 'function' ? 
                                tools.formatGNCValue(block.blockReward) : 
                                this.formatGNCValue(block.blockReward);
                        })()
                    }</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Total Reward</div>
                    <div class="detail-value">${
                        block.totalRewardTxt || 
                        (() => {
                            const tools = CTools.getInstance();
                            return tools && typeof tools.formatGNCValue === 'function' ? 
                                tools.formatGNCValue(block.totalReward) : 
                                this.formatGNCValue(block.totalReward);
                        })()
                    }</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Transactions</div>
                    <div class="detail-value">${block.transactionsCount}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">ERG Used / Limit</div>
                    <div class="detail-value">${block.ergUsedTxt || block.ergUsed} / ${block.ergLimitTxt || block.ergLimit}</div>
                </div>
            </div>
            
            ${block.transactions && block.transactions.length > 0 ? `
                <h3>Block Transactions (${block.transactions.length})</h3>
                <div class="data-table" id="block-transactions-table"></div>
            ` : ''}
        `;
        
        blockDetailsElement.innerHTML = html;
        
        // If there are transactions, initialize a table for them
        if (block.transactions && block.transactions.length > 0) {
            new Tabulator("#block-transactions-table", {
                height: "300px",
                layout: "fitColumns",
                data: block.transactions,
                columns: [
                    {title: "Transaction ID", field: "verifiableID", formatter: this.hashFormatter},
                    {title: "Status", field: "status", formatter: this.statusFormatter, width: 120},
                    {title: "From", field: "sender", formatter: this.hashFormatter},
                    {title: "To", field: "receiver", formatter: this.hashFormatter},
                    {title: "Value", field: "value", formatter: this.valueFormatter, width: 120}
                ],
                rowClick: (e, row) => {
                    if (this.callbacks && this.callbacks.onViewTransactionDetails) {
                        this.callbacks.onViewTransactionDetails(row.getData().verifiableID);
                    }
                }
            });
        }
    }
    
    /**
     * Display transaction details in the UI
     * @param {Object} tx Transaction data
     */
    displayTransactionDetails(tx) {
        const txDetailsElement = document.getElementById('transaction-details');
        if (!txDetailsElement) return;
        
        if (!tx) {
            txDetailsElement.innerHTML = '<p>No transaction details available</p>';
            return;
        }
        
        // Format the transaction data for display
        const html = `
            <div class="detail-view fade-in">
                <div class="detail-header">
                    <div class="detail-type">Transaction Details</div>
                    <div style="color: ${tx.statusColor || ''};">${tx.status || 'Unknown'}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Transaction ID</div>
                    <div class="detail-value hash-value">${tx.verifiableID}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Block</div>
                    <div class="detail-value">
                        ${tx.height > 0 ? `<span class="hash-value">${tx.blockID}</span> (Height: ${tx.height})` : 'Pending'}
                    </div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Timestamp</div>
                    <div class="detail-value">
                        ${tx.confirmedTimestamp > 0 ? new Date(tx.confirmedTimestamp * 1000).toLocaleString() : 'Pending'}
                        ${tx.unconfirmedTimestamp > 0 ? ` (Unconfirmed: ${new Date(tx.unconfirmedTimestamp * 1000).toLocaleString()})` : ''}
                    </div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">From</div>
                    <div class="detail-value hash-value">${tx.sender}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">To</div>
                    <div class="detail-value hash-value">${tx.receiver}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Value</div>
                    <div class="detail-value">${
                        tx.valueTxt || 
                        (() => {
                            const tools = CTools.getInstance();
                            return tools && typeof tools.formatGNCValue === 'function' ? 
                                tools.formatGNCValue(tx.value) : 
                                this.formatGNCValue(tx.value);
                        })()
                    }</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Transaction Fee</div>
                    <div class="detail-value">${
                        (() => {
                            const tools = CTools.getInstance();
                            return tools && typeof tools.formatGNCValue === 'function' ? 
                                tools.formatGNCValue(tx.fee) : 
                                this.formatGNCValue(tx.fee);
                        })()
                    }</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Tax</div>
                    <div class="detail-value">${
                        tx.taxTxt || 
                        (() => {
                            const tools = CTools.getInstance();
                            return tools && typeof tools.formatGNCValue === 'function' ? 
                                tools.formatGNCValue(tx.tax) : 
                                this.formatGNCValue(tx.tax);
                        })()
                    }</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Type</div>
                    <div class="detail-value">${this.getTransactionTypeName(tx.type)}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Nonce</div>
                    <div class="detail-value">${tx.nonce}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">ERG Used / Limit</div>
                    <div class="detail-value">${tx.ERGUsed} / ${tx.ERGLimit}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">ERG Price</div>
                    <div class="detail-value">${tx.ERGPrice}</div>
                </div>
                
                ${tx.log && tx.log.length > 0 ? `
                    <div class="detail-item">
                        <div class="detail-label">Transaction Log</div>
                        <div class="detail-value">
                            <pre style="background-color: rgba(0,0,0,0.3); padding: 10px; border-radius: 4px; overflow: auto; max-height: 200px;">${tx.log.join('\n')}</pre>
                        </div>
                    </div>
                ` : ''}
                
                ${tx.sourceCode ? `
                    <div class="detail-item">
                        <div class="detail-label">Source Code</div>
                        <div class="detail-value">
                            <pre style="background-color: rgba(0,0,0,0.3); padding: 10px; border-radius: 4px; overflow: auto; max-height: 300px;">${tx.sourceCode}</pre>
                        </div>
                    </div>
                ` : ''}
            </div>
        `;
        
        txDetailsElement.innerHTML = html;
    }
    
    /**
     * Display domain details in the UI
     * @param {Object} domain Domain data
     */
    displayDomainDetails(domain) {
        const domainDetailsElement = document.getElementById('domain-details');
        if (!domainDetailsElement) return;
        
        if (!domain) {
            domainDetailsElement.innerHTML = '<p>No domain details available</p>';
            return;
        }
        
        // Store the current domain for history requests
        this.currentDomain = domain.domain;
        
        // Format the domain data for display
        const html = `
            <div class="detail-view fade-in">
                <div class="detail-header">
                    <div class="detail-type">Domain Details</div>
                    <div>Transactions: ${domain.txCount || 0}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Domain</div>
                    <div class="detail-value hash-value">${domain.domain}</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Balance</div>
                    <div class="detail-value">${
                        domain.balanceTxt || 
                        (() => {
                            const tools = CTools.getInstance();
                            return tools && typeof tools.formatGNCValue === 'function' ? 
                                tools.formatGNCValue(domain.balance) : 
                                this.formatGNCValue(domain.balance);
                        })()
                    }</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Locked Balance</div>
                    <div class="detail-value">${
                        domain.lockedBalanceTxt || 
                        (() => {
                            const tools = CTools.getInstance();
                            return tools && typeof tools.formatGNCValue === 'function' ? 
                                tools.formatGNCValue(domain.lockedBalance) : 
                                this.formatGNCValue(domain.lockedBalance);
                        })()
                    }</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Total Received</div>
                    <div class="detail-value">${
                        domain.txTotalReceivedTxt || 
                        (() => {
                            const tools = CTools.getInstance();
                            return tools && typeof tools.formatGNCValue === 'function' ? 
                                tools.formatGNCValue(domain.txTotalReceived) : 
                                this.formatGNCValue(domain.txTotalReceived);
                        })()
                    }</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Total Sent</div>
                    <div class="detail-value">${
                        domain.txTotalSentTxt || 
                        (() => {
                            const tools = CTools.getInstance();
                            return tools && typeof tools.formatGNCValue === 'function' ? 
                                tools.formatGNCValue(domain.txTotalSent) : 
                                this.formatGNCValue(domain.txTotalSent);
                        })()
                    }</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Total Mined</div>
                    <div class="detail-value">${
                        domain.GNCTotalMinedTxt || 
                        (() => {
                            const tools = CTools.getInstance();
                            return tools && typeof tools.formatGNCValue === 'function' ? 
                                tools.formatGNCValue(domain.GNCTotalMined) : 
                                this.formatGNCValue(domain.GNCTotalMined);
                        })()
                    }</div>
                </div>
                
                <div class="detail-item">
                    <div class="detail-label">Genesis Rewards</div>
                    <div class="detail-value">${
                        domain.GNCTotalGenesisRewardsTxt || 
                        (() => {
                            const tools = CTools.getInstance();
                            return tools && typeof tools.formatGNCValue === 'function' ? 
                                tools.formatGNCValue(domain.GNCTotalGenesisRewards) : 
                                this.formatGNCValue(domain.GNCTotalGenesisRewards);
                        })()
                    }</div>
                </div>
                
                ${domain.perspectives && domain.perspectives.length > 0 ? `
                    <div class="detail-item">
                        <div class="detail-label">Perspectives</div>
                        <div class="detail-value">
                            ${domain.perspectives.map(p => `<span class="hash-value">${p}</span>`).join(' ')}
                        </div>
                    </div>
                    
                    <div class="detail-item">
                        <div class="detail-label">Current Perspective</div>
                        <div class="detail-value hash-value">${domain.perspective || 'None'}</div>
                    </div>
                ` : ''}
                
                ${domain.identityToken ? `
                    <div class="detail-item">
                        <div class="detail-label">Identity Token</div>
                        <div class="detail-value">Yes</div>
                    </div>
                ` : ''}
                
                ${domain.securityInfo ? `
                    <div class="detail-item">
                        <div class="detail-label">Security</div>
                        <div class="detail-value">
                            Confidence: ${domain.securityInfo.confidenceLevel}/10
                            ${domain.securityInfo.explanation ? `<br>${domain.securityInfo.explanation}` : ''}
                        </div>
                    </div>
                ` : ''}
            </div>
            
            <h3>Transaction History</h3>
            <div class="data-table" id="domain-history-table"></div>
        `;
        
        domainDetailsElement.innerHTML = html;
    }
    
    /**
     * Display search results in the UI
     * @param {Array} results Search results
     * @param {string} query Search query
     */
    displaySearchResults(results, query) {
        if (!results || results.length === 0) {
            this.showError(`No results found for: ${query}`);
            return;
        }
        
        // Check if results are all of the same type
        const resultTypes = new Set(results.map(r => r.resultType));
        
        if (resultTypes.size === 1) {
            // If all results are of the same type, show in that section
            const type = results[0].resultType;
            
            if (type === 'block' || type === this.enums.eSearchResultElemType.BLOCK) {
                this.switchToSection('blocks');
                if (this.tables.blocks) {
                    this.tables.blocks.setData(results);
                }
            } else if (type === 'transaction' || type === this.enums.eSearchResultElemType.TRANSACTION) {
                this.switchToSection('transactions');
                if (this.tables.transactions) {
                    this.tables.transactions.setData(results);
                }
            } else if (type === 'domain' || type === this.enums.eSearchResultElemType.DOMAIN) {
                this.switchToSection('domains');
                // If there's only one domain, view it directly
                if (results.length === 1 && this.callbacks && this.callbacks.onDomainSearch) {
                    this.callbacks.onDomainSearch(results[0].domain);
                } else {
                    // Display a list of domains (would need to create a domain list section)
                    this.showSearchResultsOverview(results, query);
                }
            }
        } else {
            // Mixed results, show overview
            this.showSearchResultsOverview(results, query);
        }
    }
    
    /**
     * Show search results overview for mixed result types
     * @param {Array} results Search results
     * @param {string} query Search query
     */
    showSearchResultsOverview(results, query) {
        this.switchToSection('dashboard');
        
        // Get the dashboard section
        const dashboardSection = document.getElementById('dashboard-section');
        if (!dashboardSection) return;
        
        // Create a search results container
        const searchResultsHTML = `
            <div class="search-results fade-in">
                <h2>Search Results for "${query}"</h2>
                
                ${this.hasBlockResults(results) ? `
                    <h3>Blocks</h3>
                    <div class="data-table" id="search-results-blocks"></div>
                ` : ''}
                
                ${this.hasTransactionResults(results) ? `
                    <h3>Transactions</h3>
                    <div class="data-table" id="search-results-transactions"></div>
                ` : ''}
                
                ${this.hasDomainResults(results) ? `
                    <h3>Domains</h3>
                    <div class="data-table" id="search-results-domains"></div>
                ` : ''}
            </div>
        `;
        
        // Insert search results at the beginning of the dashboard
        dashboardSection.innerHTML = searchResultsHTML + dashboardSection.innerHTML;
        
        // Initialize tables for each result type
        if (this.hasBlockResults(results)) {
            new Tabulator("#search-results-blocks", {
                height: "300px",
                layout: "fitColumns",
                data: this.getBlockResults(results),
                columns: [
                    {title: "Height", field: "height", sorter: "number", headerSortStartingDir: "desc", width: 100},
                    {title: "Block ID", field: "blockID", formatter: this.hashFormatter},
                    {title: "Miner", field: "minerID", formatter: this.hashFormatter},
                    {title: "Time", field: "solvedAt", formatter: this.timestampFormatter, width: 180},
                    {title: "Transactions", field: "transactionsCount", sorter: "number", width: 120}
                ],
                rowClick: (e, row) => {
                    if (this.callbacks && this.callbacks.onViewBlockDetails) {
                        this.callbacks.onViewBlockDetails(row.getData().blockID);
                    }
                }
            });
        }
        
        if (this.hasTransactionResults(results)) {
            new Tabulator("#search-results-transactions", {
                height: "300px",
                layout: "fitColumns",
                data: this.getTransactionResults(results),
                columns: [
                    {title: "Transaction ID", field: "verifiableID", formatter: this.hashFormatter},
                    {title: "Status", field: "status", formatter: this.statusFormatter, width: 120},
                    {title: "From", field: "sender", formatter: this.hashFormatter},
                    {title: "To", field: "receiver", formatter: this.hashFormatter},
                    {title: "Value", field: "value", formatter: this.valueFormatter, width: 120},
                    {title: "Time", field: "time", formatter: this.timestampFormatter, width: 180}
                ],
                rowClick: (e, row) => {
                    if (this.callbacks && this.callbacks.onViewTransactionDetails) {
                        this.callbacks.onViewTransactionDetails(row.getData().verifiableID);
                    }
                }
            });
        }
        
        if (this.hasDomainResults(results)) {
            new Tabulator("#search-results-domains", {
                height: "200px",
                layout: "fitColumns",
                data: this.getDomainResults(results),
                columns: [
                    {title: "Domain", field: "domain", formatter: this.hashFormatter},
                    {title: "Transactions", field: "txCount", sorter: "number", width: 120},
                    {title: "Balance", field: "balance", formatter: this.valueFormatter, width: 150}
                ],
                rowClick: (e, row) => {
                    if (this.callbacks && this.callbacks.onDomainSearch) {
                        this.callbacks.onDomainSearch(row.getData().domain);
                    }
                }
            });
        }
    }
    
    /**
     * Check if results contain blocks
     * @param {Array} results Search results
     * @returns {boolean} Has block results
     */
    hasBlockResults(results) {
        return results.some(r => 
            r.resultType === 'block' || 
            r.resultType === this.enums.eSearchResultElemType.BLOCK
        );
    }
    
    /**
     * Get block results
     * @param {Array} results Search results
     * @returns {Array} Block results
     */
    getBlockResults(results) {
        return results.filter(r => 
            r.resultType === 'block' || 
            r.resultType === this.enums.eSearchResultElemType.BLOCK
        );
    }
    
    /**
     * Check if results contain transactions
     * @param {Array} results Search results
     * @returns {boolean} Has transaction results
     */
    hasTransactionResults(results) {
        return results.some(r => 
            r.resultType === 'transaction' || 
            r.resultType === this.enums.eSearchResultElemType.TRANSACTION
        );
    }
    
    /**
     * Get transaction results
     * @param {Array} results Search results
     * @returns {Array} Transaction results
     */
    getTransactionResults(results) {
        return results.filter(r => 
            r.resultType === 'transaction' || 
            r.resultType === this.enums.eSearchResultElemType.TRANSACTION
        );
    }
    
    /**
     * Check if results contain domains
     * @param {Array} results Search results
     * @returns {boolean} Has domain results
     */
    hasDomainResults(results) {
        return results.some(r => 
            r.resultType === 'domain' || 
            r.resultType === this.enums.eSearchResultElemType.DOMAIN
        );
    }
    
    /**
     * Get domain results
     * @param {Array} results Search results
     * @returns {Array} Domain results
     */
    getDomainResults(results) {
        return results.filter(r => 
            r.resultType === 'domain' || 
            r.resultType === this.enums.eSearchResultElemType.DOMAIN
        );
    }
    
    /**
     * Get transaction type name
     * @param {number} type Transaction type
     * @returns {string} Transaction type name
     */
    getTransactionTypeName(type) {
        if (this.enums.eTXType) {
            // Try to find a matching enum value
            for (const [name, value] of Object.entries(this.enums.eTXType)) {
                if (value === type) {
                    return name.replace(/_/g, ' ');
                }
            }
        }
        
        // Fallback type names
        switch (type) {
            case 0: return "TRANSFER";
            case 1: return "BLOCK REWARD";
            case 2: return "OFF CHAIN";
            case 3: return "OFF CHAIN CASH OUT";
            case 4: return "CONTRACT";
            default: return "UNKNOWN";
        }
    }
    
    // Table formatter methods
    
    /**
     * Format hash values for display in tables
     * @param {Object} cell Cell value
     * @returns {string} Formatted HTML
     */
    hashFormatter(cell) {
        const value = cell.getValue();
        if (!value) return "";
        
        const shortValue = value.length > 10 ? value.substring(0, 7) + '...' + value.substring(value.length - 3) : value;
        return `<span class="hash-value" title="${value}">${shortValue}</span>`;
    }
    
    /**
     * Format status values for display in tables
     * @param {Object} cell Cell value
     * @returns {string} Formatted HTML
     */
    statusFormatter(cell) {
        const value = cell.getValue();
        if (!value) return "";
        
        let color;
        
        switch (value.toLowerCase()) {
            case "finalized (safe)":
            case "finalized":
            case "success":
                color = "var(--neon-green)";
                break;
            case "finalized (pending)":
            case "pending":
            case "processing":
                color = "var(--cyberpunk-yellow)";
                break;
            case "unconfirmed":
            case "in mempool":
                color = "var(--neon-blue)";
                break;
            case "failed":
            case "rejected":
            case "error":
                color = "#ff3860";
                break;
            default:
                color = "var(--text-primary)";
        }
        
        return `<span style="color: ${color}; font-weight: bold;">${value}</span>`;
    }
    
    /**
     * Format value (currency) for display in tables
     * @param {Object} cell Cell value
     * @returns {string} Formatted value
     */
    valueFormatter(cell) {
        let value = cell.getValue();
        
        // If value is null or undefined, return empty string
        if (value == null) return "";
        
        // If value is already a string with GNC suffix, assume it's already formatted
        if (typeof value === 'string' && value.includes('GNC')) return value;
        
        // Try to format the value using CTools.formatGNCValue
        try {
            const tools = CTools.getInstance();
            if (tools && typeof tools.formatGNCValue === 'function') {
                // This will include the appropriate suffix (T GNC, A GNC, etc.)
                return tools.formatGNCValue(value);
            } else {
                // Use our class method as fallback
                return this.formatGNCValue(value);
            }
        } catch (e) {
            // If unable to format, return the original value as string
            return String(value) + " GNC";
        }
    }
    
    /**
     * Format timestamp for display in tables
     * @param {Object} cell Cell value
     * @returns {string} Formatted timestamp
     */
    timestampFormatter(cell) {
        const timestamp = cell.getValue();
        if (!timestamp) return "";
        
        try {
            // Handle timestamps in seconds (blockchain standard)
            const date = new Date(typeof timestamp === 'number' && timestamp < 2000000000 ? 
                timestamp * 1000 : timestamp);
            return date.toLocaleString();
        } catch (e) {
            return String(timestamp);
        }
    }
    
    /**
     * Format size for display in tables
     * @param {Object} cell Cell value
     * @returns {string} Formatted size
     */
    sizeFormatter(cell) {
        const bytes = cell.getValue();
        if (bytes == null) return "";
        
        return this.formatSize(bytes);
    }
    
    /**
     * Format a GNC value consistently using CTools.formatGNCValue
     * @param {BigInt|string|number} value The GNC value to format in attoGNC units
     * @param {number} precision Number of decimal places to show
     * @returns {string} Formatted GNC value with appropriate suffix
     */
    formatGNCValue(value, precision = 5) {
        try {
            const tools = CTools.getInstance();
            
            // Use the system tools.formatGNCValue if available
            if (tools && typeof tools.formatGNCValue === 'function') {
                // This function already includes appropriate suffix (T GNC, A GNC, etc.)
                return tools.formatGNCValue(value, precision);
            }
            
            // Fallback implementation if tools.formatGNCValue is not available
            if (typeof value === 'bigint') {
                // Basic GNC formatting: 1 GNC = 10^18 atto
                const divisor = 10n ** 18n;
                const integerPart = value / divisor;
                const fractionalPart = value % divisor;
                
                // Format fractional part with proper padding
                let fractionalStr = fractionalPart.toString().padStart(18, '0');
                fractionalStr = fractionalStr.substring(0, precision);
                
                return `${integerPart}.${fractionalStr} GNC`;
            } else if (typeof value === 'number') {
                return `${value.toFixed(precision)} GNC`;
            }
            
            // If it's already a string or other type, just convert to string
            return `${String(value)} GNC`;
        } catch (error) {
            console.warn("Error formatting GNC value:", error);
            return `${String(value)} GNC`;
        }
    }
    
    /**
     * Handle window resize event
     * @param {number} width Window width
     * @param {number} height Window height
     */
    handleResize(width, height) {
        // Update table heights based on window size
        if (height < 600) {
            // Mobile layout
            if (this.tables.recentBlocks) this.tables.recentBlocks.setHeight(200);
            if (this.tables.recentTransactions) this.tables.recentTransactions.setHeight(200);
            if (this.tables.blocks) this.tables.blocks.setHeight(400);
            if (this.tables.transactions) this.tables.transactions.setHeight(400);
            if (this.tables.domainHistory) this.tables.domainHistory.setHeight(300);
        } else {
            // Desktop layout
            if (this.tables.recentBlocks) this.tables.recentBlocks.setHeight(300);
            if (this.tables.recentTransactions) this.tables.recentTransactions.setHeight(300);
            if (this.tables.blocks) this.tables.blocks.setHeight(600);
            if (this.tables.transactions) this.tables.transactions.setHeight(600);
            if (this.tables.domainHistory) this.tables.domainHistory.setHeight(400);
        }
        
        // Redraw tables to adjust to new size
        Object.values(this.tables).forEach(table => {
            if (table && typeof table.redraw === 'function') {
                table.redraw();
            }
        });
        
        // Update charts to fit new size
        if (this.charts.transactionsDaily) {
            try {
                Plotly.relayout('transactions-chart', {
                    width: width - 40,
                    height: 300
                });
            } catch (e) {
                console.warn("Error resizing transactions chart", e);
            }
        }
        
        if (this.charts.dailyStats) {
            try {
                Plotly.relayout('daily-stats-chart', {
                    width: width - 40,
                    height: 300
                });
            } catch (e) {
                console.warn("Error resizing daily stats chart", e);
            }
        }
        
        // Handle sidebar responsiveness
        if (this.elements && this.elements.sidebar) {
            if (width < 768) {
                this.elements.sidebar.classList.add('sidebar-collapsed');
            } else {
                this.elements.sidebar.classList.remove('sidebar-collapsed');
            }
        }
    }
    
    /**
     * Clean up UI resources
     */
    cleanup() {
        // Destroy tables
        Object.values(this.tables).forEach(table => {
            if (table && typeof table.destroy === 'function') {
                table.destroy();
            }
        });
        
        // Clear table references
        this.tables = {};
        
        // Destroy charts
        if (this.charts.transactionsDaily) {
            try {
                Plotly.purge('transactions-chart');
            } catch (e) {
                console.warn("Error purging transactions chart", e);
            }
        }
        
        if (this.charts.dailyStats) {
            try {
                Plotly.purge('daily-stats-chart');
            } catch (e) {
                console.warn("Error purging daily stats chart", e);
            }
        }
        
        // Clear chart references
        this.charts = {};
        
        // Clear callbacks
        this.callbacks = null;
    }
    
    // Callback handlers
    
    /**
     * View block details handler
     * @param {string} blockID Block ID
     */
    onViewBlockDetails(blockID) {
        if (this.callbacks && this.callbacks.onViewBlockDetails) {
            this.callbacks.onViewBlockDetails(blockID);
        }
    }
    
    /**
     * View transaction details handler
     * @param {string} transactionID Transaction ID
     */
    onViewTransactionDetails(transactionID) {
        if (this.callbacks && this.callbacks.onViewTransactionDetails) {
            this.callbacks.onViewTransactionDetails(transactionID);
        }
    }
}

// Initialize settings
CUIBlockchainExplorer.sCurrentSettings = new CAppSettings(CUIBlockchainExplorer.getPackageID());

// Export the class as default
export default CUIBlockchainExplorer;