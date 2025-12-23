/**
 * Search Configuration Panel Component
 * 
 * A UI component for configuring search options, including standard and arbitrary flags
 */
class SearchConfigPanel {
    /**
     * Construct a new SearchConfigPanel
     * @param {HTMLElement} container - Container element for the panel
     * @param {Object} enums - Enums used for flags
     * @param {Function} onChange - Callback when configuration changes
     */
    constructor(container, enums, onChange) {
        this.container = container;
        this.enums = enums;
        this.onChange = onChange;
        
        // Track current state
        this.standardFlags = {
            transactions: true,
            blocks: true,
            domains: true,
            addresses: true
        };
        
        this.arbitraryFlags = [];
        
        // Initialize the panel
        this.initialize();
    }
    
    /**
     * Initialize the search panel
     */
    initialize() {
        // Create panel HTML
        this.container.innerHTML = `<style>
		/* Cyberpunk Checkbox Styling */
.checkbox-group {
  display: flex;
  flex-wrap: wrap;
  gap: 0.8rem;
  margin: 1rem 0;
}

.checkbox-label {
  position: relative;
  padding-left: 2.5rem;
  cursor: pointer;
  display: flex;
  align-items: center;
  font-size: 0.9rem;
  color: #b3b3cc;
  transition: color 0.3s ease;
  margin-right: 0.5rem;
  user-select: none;
}

.checkbox-label:hover {
  color: #ffffff;
}

/* Hide the default checkbox */
.checkbox-label input[type="checkbox"] {
  opacity: 0;
  position: absolute;
}

/* Create custom checkbox */
.checkbox-label input[type="checkbox"] + span {
  position: absolute;
  left: 0;
  top: 50%;
  transform: translateY(-50%);
  width: 1.6rem;
  height: 1.6rem;
  background-color: #050714;
  border: 1px solid rgba(59, 110, 255, 0.3);
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 0 0 5px rgba(0, 0, 0, 0.3) inset;
  transition: all 0.2s ease;
}

/* Checked state */
.checkbox-label input[type="checkbox"]:checked + span {
  border-color: #3d6eff;
  background-color: rgba(61, 110, 255, 0.2);
  box-shadow: 0 0 8px #3d6eff;
}

/* Checkmark */
.checkbox-label input[type="checkbox"] + span:after {
  content: '';
  position: absolute;
  display: none;
  width: 0.5rem;
  height: 1rem;
  border: solid #0cffff;
  border-width: 0 2px 2px 0;
  transform: rotate(45deg);
  top: 0.2rem;
  left: 0.5rem;
  opacity: 0;
  transition: opacity 0.2s ease;
}

/* Display checkmark when checked */
.checkbox-label input[type="checkbox"]:checked + span:after {
  display: block;
  opacity: 1;
}

/* Focus state for accessibility */
.checkbox-label input[type="checkbox"]:focus + span {
  border-color: #b935f8;
  box-shadow: 0 0 8px #b935f8;
}

/* Hover effects */
.checkbox-label:hover input[type="checkbox"] + span {
  border-color: #3d6eff;
  box-shadow: 0 0 5px #3d6eff;
}

/* Different colors for different types */
.checkbox-label[for="flag-transactions"]:hover input[type="checkbox"] + span,
.checkbox-label[for="flag-transactions"] input[type="checkbox"]:checked + span {
  border-color: #b935f8;
  box-shadow: 0 0 8px #b935f8;
}

.checkbox-label[for="flag-blocks"]:hover input[type="checkbox"] + span,
.checkbox-label[for="flag-blocks"] input[type="checkbox"]:checked + span {
  border-color: #3d6eff;
  box-shadow: 0 0 8px #3d6eff;
}

.checkbox-label[for="flag-domains"]:hover input[type="checkbox"] + span,
.checkbox-label[for="flag-domains"] input[type="checkbox"]:checked + span {
  border-color: #0cff0c;
  box-shadow: 0 0 8px #0cff0c;
}

.checkbox-label[for="flag-addresses"]:hover input[type="checkbox"] + span,
.checkbox-label[for="flag-addresses"] input[type="checkbox"]:checked + span {
  border-color: #0cffff;
  box-shadow: 0 0 8px #0cffff;
}</style>
            <div class="search-config-panel">
                <div class="standard-flags">
                    <div class="config-section-title">Search in:</div>
                    <div class="checkbox-group">
    <label class="checkbox-label" for="flag-transactions">
        <input type="checkbox" id="flag-transactions" checked> 
        <span></span>
        Transactions
    </label>
    <label class="checkbox-label" for="flag-blocks">
        <input type="checkbox" id="flag-blocks" checked> 
        <span></span>
        Blocks
    </label>
    <label class="checkbox-label" for="flag-domains">
        <input type="checkbox" id="flag-domains" checked> 
        <span></span>
        Domains
    </label>
    <label class="checkbox-label" for="flag-addresses">
        <input type="checkbox" id="flag-addresses" checked> 
        <span></span>
        Addresses
    </label>
</div>
                </div>
                
                <div class="arbitrary-flags">
                    <div class="config-section-title">Advanced Filters:</div>
                    <div id="arbitrary-flags-container"></div>
                    <button id="add-flag-btn" class="search-button">Add Filter</button>
                </div>
            </div>
            
            <style>
                .search-config-panel {
                    background-color: var(--panel-bg);
                    border: 1px solid var(--panel-border);
                    border-radius: 8px;
                    padding: 1rem;
                    margin-bottom: 1rem;
                }
                
                .config-section-title {
                    font-weight: bold;
                    margin-bottom: 0.5rem;
                    color: #9ecfdb;
                }
                
                .checkbox-group {
                    display: flex;
                    flex-wrap: wrap;
                    gap: 1rem;
                    margin-bottom: 1rem;
                }
                
                .checkbox-label {
                    display: flex;
                    align-items: center;
                    cursor: pointer;
                }
                
                .checkbox-label input {
                    margin-right: 0.5rem;
                }
                
                .flag-row {
                    display: flex;
                    gap: 0.5rem;
                    margin-bottom: 0.5rem;
                    align-items: center;
                }
                
                .flag-row select, .flag-row input {
                    background-color: #0b253d;
                    border: 1px solid #174141;
                    color: #7dc5db;
                    padding: 0.3rem 0.5rem;
                    border-radius: 4px;
                }
                
                .flag-row button {
                    background-color: rgba(255, 56, 96, 0.2);
                    border: 1px solid rgba(255, 56, 96, 0.4);
                    color: #ff3860;
                    border-radius: 4px;
                    padding: 0.3rem 0.5rem;
                    cursor: pointer;
                }
                
                .flag-row button:hover {
                    background-color: rgba(255, 56, 96, 0.3);
                }
            </style>
        `;
        
        // Set up event listeners
        this.setupEventListeners();
        
        // Initialize arbitrary flags container
        this.arbitraryFlagsContainer = this.container.querySelector('#arbitrary-flags-container');
    }
    
    /**
     * Set up event listeners for the panel
     */
    setupEventListeners() {
        // Standard flags checkboxes
        const transactionsCheckbox = this.container.querySelector('#flag-transactions');
        const blocksCheckbox = this.container.querySelector('#flag-blocks');
        const domainsCheckbox = this.container.querySelector('#flag-domains');
        const addressesCheckbox = this.container.querySelector('#flag-addresses');
        
        transactionsCheckbox.addEventListener('change', () => {
            this.standardFlags.transactions = transactionsCheckbox.checked;
            this.notifyChange();
        });
        
        blocksCheckbox.addEventListener('change', () => {
            this.standardFlags.blocks = blocksCheckbox.checked;
            this.notifyChange();
        });
        
        domainsCheckbox.addEventListener('change', () => {
            this.standardFlags.domains = domainsCheckbox.checked;
            this.notifyChange();
        });
        
        addressesCheckbox.addEventListener('change', () => {
            this.standardFlags.addresses = addressesCheckbox.checked;
            this.notifyChange();
        });
        
        // Add flag button
        const addFlagBtn = this.container.querySelector('#add-flag-btn');
        addFlagBtn.addEventListener('click', () => {
            this.addArbitraryFlag();
        });
    }
    
    /**
     * Add a new arbitrary flag row
     */
    addArbitraryFlag() {
        const flagId = Date.now(); // Unique id for the row
        const flagRow = document.createElement('div');
        flagRow.className = 'flag-row';
        flagRow.dataset.id = flagId;
        
        // Create property type select (transaction, block, domain)
        const typeSelect = document.createElement('select');
        typeSelect.innerHTML = `
            <option value="tx">Transaction</option>
            <option value="block">Block</option>
            <option value="domain">Domain</option>
        `;
        
        // Create property select based on type
        const propertySelect = document.createElement('select');
        this.updatePropertyOptions(propertySelect, 'tx');
        
        // Create value input
        const valueInput = document.createElement('input');
        valueInput.type = 'text';
        valueInput.placeholder = 'Value (optional)';
        
        // Create remove button
        const removeBtn = document.createElement('button');
        removeBtn.textContent = 'Remove';
        removeBtn.addEventListener('click', () => {
            flagRow.remove();
            this.arbitraryFlags = this.arbitraryFlags.filter(f => f.id !== flagId);
            this.notifyChange();
        });
        
        // Add to row
        flagRow.appendChild(typeSelect);
        flagRow.appendChild(propertySelect);
        flagRow.appendChild(valueInput);
        flagRow.appendChild(removeBtn);
        
        // Add to container
        this.arbitraryFlagsContainer.appendChild(flagRow);
        
        // Add to state
        this.arbitraryFlags.push({
            id: flagId,
            type: 'tx',
            property: this.getFirstProperty('tx'),
            value: ''
        });
        
        // Set up event listeners
        typeSelect.addEventListener('change', () => {
            const type = typeSelect.value;
            this.updatePropertyOptions(propertySelect, type);
            
            // Update state
            const flagIndex = this.arbitraryFlags.findIndex(f => f.id === flagId);
            if (flagIndex !== -1) {
                this.arbitraryFlags[flagIndex].type = type;
                this.arbitraryFlags[flagIndex].property = this.getFirstProperty(type);
                this.notifyChange();
            }
        });
        
        propertySelect.addEventListener('change', () => {
            // Update state
            const flagIndex = this.arbitraryFlags.findIndex(f => f.id === flagId);
            if (flagIndex !== -1) {
                this.arbitraryFlags[flagIndex].property = propertySelect.value;
                this.notifyChange();
            }
        });
        
        valueInput.addEventListener('input', () => {
            // Update state
            const flagIndex = this.arbitraryFlags.findIndex(f => f.id === flagId);
            if (flagIndex !== -1) {
                this.arbitraryFlags[flagIndex].value = valueInput.value;
                this.notifyChange();
            }
        });
    }
    
    /**
     * Update property select options based on type
     * @param {HTMLSelectElement} select - Select element to update
     * @param {string} type - Type (tx, block, domain)
     */
    updatePropertyOptions(select, type) {
        select.innerHTML = '';
        
        let options = [];
        
        switch (type) {
            case 'tx':
                options = [
                    { value: 'sender', label: 'Sender' },
                    { value: 'receiver', label: 'Receiver' },
                    { value: 'value', label: 'Value' },
                    { value: 'height', label: 'Block Height' },
                    { value: 'type', label: 'Transaction Type' },
                    { value: 'result', label: 'Transaction Result' }
                ];
                break;
                
            case 'block':
                options = [
                    { value: 'height', label: 'Height' },
                    { value: 'keyHeight', label: 'Key Height' },
                    { value: 'minerID', label: 'Miner ID' },
                    { value: 'type', label: 'Block Type' },
                    { value: 'difficulty', label: 'Difficulty' },
                    { value: 'solvedAt', label: 'Timestamp' }
                ];
                break;
                
            case 'domain':
                options = [
                    { value: 'domain', label: 'Domain Address' },
                    { value: 'txCount', label: 'Transaction Count' },
                    { value: 'balance', label: 'Balance' },
                    { value: 'perspectivesCount', label: 'Perspectives Count' },
                    { value: 'perspective', label: 'Current Perspective' }
                ];
                break;
        }
        
        options.forEach(option => {
            const optionElement = document.createElement('option');
            optionElement.value = option.value;
            optionElement.textContent = option.label;
            select.appendChild(optionElement);
        });
    }
    
    /**
     * Get the first property for a type
     * @param {string} type - Type (tx, block, domain)
     * @returns {string} First property
     */
    getFirstProperty(type) {
        switch (type) {
            case 'tx': return 'sender';
            case 'block': return 'height';
            case 'domain': return 'domain';
            default: return '';
        }
    }
    
    /**
     * Notify about changes
     */
    notifyChange() {
        if (this.onChange) {
            this.onChange({
                standardFlags: this.standardFlags,
                arbitraryFlags: this.arbitraryFlags
            });
        }
    }
    
/**
 * Create a CSearchFilter from the current configuration
 * @param {CSearchFilter} CSearchFilter - CSearchFilter class
 * @returns {CSearchFilter} Configured search filter
 */
createSearchFilter(CSearchFilter) {
    try {
        const filter = new CSearchFilter();
        
        // Add standard flags
        const standardFlags = [];
        if (this.standardFlags.transactions) {
            standardFlags.push(CSearchFilter.StandardFlags.TRANSACTIONS);
        }
        if (this.standardFlags.blocks) {
            standardFlags.push(CSearchFilter.StandardFlags.BLOCKS);
        }
        if (this.standardFlags.domains) {
            standardFlags.push(CSearchFilter.StandardFlags.DOMAINS);
        }
        if (this.standardFlags.addresses) {
            standardFlags.push(CSearchFilter.StandardFlags.ADDRESSES);
        }
        
        if (standardFlags.length > 0) {
            filter.setStandardFlags(...standardFlags);
        }
        
        // Group arbitrary flags by name and prioritize those with values
        const groupedFlags = {};
        for (const flag of this.arbitraryFlags) {
            const flagName = `${flag.type}_${flag.property}`;
            
            // If this flag name doesn't exist yet in our grouping, add it
            if (!groupedFlags[flagName]) {
                groupedFlags[flagName] = flag;
            } 
            // If it exists but has no value, and the current flag has a value, replace it
            else if (flag.value && !groupedFlags[flagName].value) {
                groupedFlags[flagName] = flag;
            }
        }
        
        // Add prioritized arbitrary flags
        for (const [flagName, flag] of Object.entries(groupedFlags)) {
            if (flag.value) {
                filter.setArbitraryFlag(flagName, flag.value);
            } else {
                filter.setArbitraryFlag(flagName, true);
            }
        }
        
        return filter;
    } catch (error) {
        console.error("Error creating search filter:", error);
        return null;
    }
}
    
    /**
     * Toggle visibility of the panel
     * @param {boolean} visible - Whether the panel should be visible
     */
    setVisible(visible) {
        this.container.style.display = visible ? 'block' : 'none';
    }
    
    /**
     * Reset the panel to default state
     */
    reset() {
        // Reset standard flags
        this.standardFlags = {
            transactions: true,
            blocks: true,
            domains: true,
            addresses: true
        };
        
        // Update checkboxes
        this.container.querySelector('#flag-transactions').checked = true;
        this.container.querySelector('#flag-blocks').checked = true;
        this.container.querySelector('#flag-domains').checked = true;
        this.container.querySelector('#flag-addresses').checked = true;
        
        // Clear arbitrary flags
        this.arbitraryFlags = [];
        this.arbitraryFlagsContainer.innerHTML = '';
        
        // Notify change
        this.notifyChange();
    }
}

export default SearchConfigPanel;