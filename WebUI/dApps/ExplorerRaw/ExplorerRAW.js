/**
 * GRIDNET Blockchain Explorer UI dApp
 *
 * A modern, maintainable implementation of the blockchain explorer for GRIDNET OS.
 * This implementation follows a modular approach with clear separation of concerns.
 */

//Imports - BEGIN
import {
    CWindow
}
from "/lib/window.js"
import {
    CVMMetaSection,
    CVMMetaEntry,
    CVMMetaGenerator,
    CVMMetaParser
}
from '/lib/MetaData.js'
import {
    CNetMsg
}
from '/lib/NetMsg.js'
import {
    CTools,
    CDataConcatenator
}
from '/lib/tools.js'
import {
    CAppSettings,
    CSettingsManager
}
from "/lib/SettingsManager.js"
import {
    CContentHandler
}
from "/lib/AppSelector.js"
import {
    CSearchFilter
}
from "/lib/SearchFilter.js"
import {
    CGLink,
    CGLinkHandler
}
from "/lib/GLink.js"

//Imports - END

//Window Body - BEGIN
var blockchainExplorerBody = `<link rel="stylesheet" href="/css/windowDefault.css" />
<link rel="stylesheet" href="/CSS/jquery.contextMenu.min.css" />
<link rel="stylesheet" href="/css/tabulator.min.css" />

<style>
  body {
  background-color: black;
  color: #14c5cf !important;
}

.hovered-cell {
    background-color: rgba(61, 110, 255, 0.2) !important;
    transition: background-color 0.2s ease;
}
  /* Global Styles */
  .blockchain-explorer {
    font-family: 'Consolas', 'Courier New', monospace;
    background-color: #0a0b16;
    color: #ffffff;
    width: 100%;
    height: 100%;
    display: flex;
    flex-direction: column;
    overflow: hidden;
  }

  /* Typography */
  h1, h2, h3, h4 {
    font-family: 'Orbitron', 'Arial', sans-serif;
    letter-spacing: 1px;
    margin: 0;
    padding: 0.5rem 0;
  }

  h1 {
    font-size: 1.5rem;
    background: linear-gradient(to right, #3d6eff, #b935f8);
    -webkit-background-clip: text;
    background-clip: text;
    -webkit-text-fill-color: transparent;
    text-shadow: 0 0 5px rgba(61, 110, 255, 0.7);
  }

  h2 {
    font-size: 1.2rem;
    color: #0cffff;
    text-shadow: 0 0 5px rgba(61, 110, 255, 0.7);
  }

  /* Header Section */
  .header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0.5rem 1rem;
    border-bottom: 1px solid #3d6eff;
    background-color: #050714;
    min-height: 3.5rem;
  }

  .header-title {
    display: flex;
    align-items: center;
  }

  .header-status {
    display: flex;
    align-items: center;
    font-size: 0.8rem;
  }

  .status-dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    margin-right: 8px;
  }

  .status-live {
    background-color: #0cff0c;
    box-shadow: 0 0 5px #0cff0c;
    animation: pulse 1.5s infinite;
  }

  .status-offline {
    background-color: #ff3860;
    box-shadow: 0 0 5px #ff3860;
  }

  @keyframes pulse {
    0% { opacity: 1; }
    50% { opacity: 0.5; }
    100% { opacity: 1; }
  }

  /* Search Section */
  .search-section {
    padding: 0.5rem 1rem;
    display: flex;
    align-items: center;
    background-color: #050714;
    border-bottom: 1px solid rgba(59, 110, 255, 0.3);
  }

  .search-input {
    flex: 1;
    background-color: rgba(10, 11, 22, 0.7);
    border: 1px solid rgba(59, 110, 255, 0.3);
    border-radius: 4px;
    padding: 0.5rem;
    color: #ffffff;
    font-family: inherit;
    margin-right: 0.5rem;
  }

  .search-input:focus {
    outline: none;
    border-color: #b935f8;
    box-shadow: 0 0 5px rgba(185, 53, 248, 0.7);
  }

  .search-button {
    background: linear-gradient(to right, #3d6eff, #b935f8);
    border: none;
    border-radius: 4px;
    color: #f6fbac;
    padding: 0.5rem 1rem;
    cursor: pointer;
    font-weight: bold;
    transition: all 0.3s ease;
  }

  .search-button:hover {
    box-shadow: 0 0 5px rgba(185, 53, 248, 0.7);
  }

  /* Main Content Area */
  .main-content {
    display: flex;
    flex: 1;
    overflow: hidden;
  }

  /* Sidebar */
  .sidebar {
    width: 250px;
    background-color: #050714;
    border-right: 1px solid rgba(59, 110, 255, 0.3);
    overflow-y: auto;
    transition: width 0.3s ease;
  }

  .sidebar-collapsed {
    width: 50px;
  }

  .nav-item {
    padding: 0.8rem 1rem;
    cursor: pointer;
    transition: background-color 0.3s ease;
    display: flex;
    align-items: center;
    color: #b3b3cc;
  }

  .nav-item:hover {
    background-color: rgba(61, 110, 255, 0.1);
    color: #ffffff;
  }

  .nav-item.active {
    background-color: rgba(61, 110, 255, 0.2);
    color: #3d6eff;
    border-left: 3px solid #3d6eff;
  }

  .nav-icon {
    margin-right: 10px;
    width: 18px;
    text-align: center;
  }

  /* Content Sections */
  .content-area {
    flex: 1;
    overflow-y: auto;
    padding: 1rem;
    position: relative;
  }

  .content-section {
    display: none;
    flex-direction: column;
    gap: 1rem;
  }

  .content-section.active {
    display: flex;
  }

 /* Holographic Effect - BEGIN */
 .hologram-header {
  position: relative;
  margin-bottom: 1.5rem;
  padding: 0.7rem 1rem;
  border: 1px solid rgba(61, 110, 255, 0.3);
  border-radius: 4px;
  overflow: hidden;
}

.hologram-text {
  position: relative;
  color: var(--neon-cyan);
  font-family: 'Orbitron', sans-serif;
  letter-spacing: 3px;
  text-transform: uppercase;
  text-shadow: 0 0 5px var(--neon-cyan);
  z-index: 2;
  animation: hologram-shake 0.5s infinite alternate;
  opacity: 0.9;
}

.hologram-light {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: linear-gradient(45deg, 
    rgba(12, 255, 255, 0) 0%, 
    rgba(12, 255, 255, 0.05) 50%, 
    rgba(12, 255, 255, 0) 100%
  );
  animation: hologram-rotate 10s linear infinite;
  z-index: 1;
}

.hologram-flicker {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: 3;
  pointer-events: none;
  animation: hologram-flicker 6s linear infinite;
  background: transparent;
}

@keyframes hologram-shake {
  0%, 100% { transform: translateX(0); }
  10% { transform: translateX(-1px); }
  30% { transform: translateX(1px); }
  50% { transform: translateX(-0.5px); }
  70% { transform: translateX(0.5px); }
}

@keyframes hologram-rotate {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

@keyframes hologram-flicker {
  0%, 100% { opacity: 0; }
  92% { opacity: 0; }
  93% { opacity: 0.1; }
  94% { opacity: 0; }
  95% { opacity: 0.3; }
  96% { opacity: 0; }
  97% { opacity: 0.2; }
  98% { opacity: 0; }
  99% { opacity: 0.1; }
}
 /* Holographic Effect - END */
  /* Dashboard Panels */
  .stats-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
    gap: 1rem;
    margin-bottom: 1rem;
  }

  .stat-panel {
    background-color: rgba(10, 11, 22, 0.7);
    border: 1px solid rgba(59, 110, 255, 0.3);
    border-radius: 8px;
    padding: 1rem;
    transition: all 0.3s ease;
  }

  .stat-panel:hover {
    box-shadow: 0 0 5px rgba(61, 110, 255, 0.7);
    border-color: #3d6eff;
  }

  .stat-title {
    font-size: 0.85rem;
    color: #b3b3cc;
    margin-bottom: 0.5rem;
  }

  .stat-value {
    font-size: 1.5rem;
    font-weight: bold;
    color: #0cffff;
  }

  .stat-change {
    font-size: 0.75rem;
    display: flex;
    align-items: center;
    margin-top: 0.25rem;
  }

  .stat-change.positive {
    color: #0cff0c;
  }

  .stat-change.negative {
    color: #ff3860;
  }

  /* Charts and Graphs */
  .chart-container {
    background-color: rgba(10, 11, 22, 0.7);
    border: 1px solid rgba(59, 110, 255, 0.3);
    border-radius: 8px;
    padding: 1rem;
    margin-bottom: 1rem;
    height: 300px;
  }

  /* Tables */
  .data-table {
    width: 100%;
    margin-bottom: 1rem;
  }
  
  /* Custom Tabulator Styling */
  .tabulator {
    background-color: transparent !important;
    border: 1px solid rgba(59, 110, 255, 0.3) !important;
    border-radius: 8px;
    overflow: hidden;
  }

  .tabulator-header {
    background-color: #050714 !important;
    border-bottom: 2px solid #3d6eff !important;
  }

  .tabulator-col {
    background-color: transparent !important;
    border-right: 1px solid rgba(59, 110, 255, 0.3) !important;
  }

  .tabulator-col-title {
    color: #51adf1  !important;
    font-weight: bold !important;
  }

  .tabulator-row {
    background-color: rgba(10, 11, 22, 0.7) !important;
    border-bottom: 1px solid rgba(59, 110, 255, 0.3) !important;
    transition: all 0.3s ease;
  }

  .tabulator-row:hover {
    background-color: rgba(61, 110, 255, 0.2) !important;
  }

  .tabulator-cell {
    color: #ffffff !important;
    border-right: 1px solid rgba(59, 110, 255, 0.3) !important;
  }

  /* Detail View Styling */
  .detail-view {
    background-color: rgba(10, 11, 22, 0.7);
    border: 1px solid rgba(59, 110, 255, 0.3);
    border-radius: 8px;
    padding: 1rem;
    margin-bottom: 1rem;
  }

  .detail-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 1rem;
    padding-bottom: 0.5rem;
    border-bottom: 1px solid rgba(59, 110, 255, 0.3);
  }


.terminal-header {
  position: relative;
  margin-bottom: 1.5rem;
  padding: 0.5rem 0;
  display: flex;
  align-items: center;
  font-family: 'Courier New', monospace;
}

.terminal-prefix {
  color: #01ff01;
  margin-right: 0.5rem;
  font-weight: bold;
}

.terminal-text {
  color: #0abf0a;
  overflow: hidden;
  white-space: nowrap;
  border-right: none;
  animation: typing 2s steps(20, end) infinite alternate;
  font-family: 'Courier New', monospace;
  margin: 0;
  font-size: 1.5rem;
}

.terminal-cursor {
  display: inline-block;
  width: 10px;
  height: 1.5rem;
  background-color: #00ff00;
  margin-left: 2px;
  animation: blink 1s step-end infinite;
}

@keyframes typing {
  from { width: 0; }
  to { width: 100%; }
}

@keyframes blink {
  0%, 100% { opacity: 1; }
  50% { opacity: 0; }
}

/* ========================================
   Horizontal Sub-tabs (Cyberpunk Theme)
   ======================================== */
.blocks-subtabs {
  margin-bottom: 1.5rem;
  position: relative;
}

.subtab-nav {
  display: flex;
  gap: 0.5rem;
  background: linear-gradient(180deg, rgba(16, 5, 28, 0.9) 0%, rgba(10, 2, 20, 0.95) 100%);
  border: 1px solid rgba(185, 53, 248, 0.3);
  border-radius: 8px;
  padding: 0.5rem;
  position: relative;
  overflow: hidden;
}

.subtab-nav::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 1px;
  background: linear-gradient(90deg, transparent, rgba(185, 53, 248, 0.5), rgba(0, 212, 255, 0.5), transparent);
}

.subtab-btn {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.75rem 1.5rem;
  background: transparent;
  border: 1px solid transparent;
  border-radius: 6px;
  color: #8a8aad;
  font-family: 'Courier New', monospace;
  font-size: 0.9rem;
  font-weight: 500;
  cursor: pointer;
  position: relative;
  transition: all 0.3s ease;
  overflow: hidden;
}

.subtab-btn i {
  font-size: 1rem;
  transition: all 0.3s ease;
}

.subtab-btn span {
  position: relative;
  z-index: 1;
}

.subtab-glow {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: radial-gradient(ellipse at center, rgba(185, 53, 248, 0.15) 0%, transparent 70%);
  opacity: 0;
  transition: opacity 0.3s ease;
  pointer-events: none;
}

.subtab-btn:hover {
  color: #c967f7;
  border-color: rgba(185, 53, 248, 0.3);
  background: rgba(185, 53, 248, 0.1);
}

.subtab-btn:hover .subtab-glow {
  opacity: 1;
}

.subtab-btn:hover i {
  color: #00d4ff;
  text-shadow: 0 0 10px rgba(0, 212, 255, 0.5);
}

.subtab-btn.active {
  color: #00d4ff;
  background: linear-gradient(135deg, rgba(185, 53, 248, 0.2) 0%, rgba(0, 212, 255, 0.15) 100%);
  border-color: rgba(0, 212, 255, 0.5);
  box-shadow: 0 0 15px rgba(0, 212, 255, 0.2), inset 0 0 20px rgba(185, 53, 248, 0.1);
}

.subtab-btn.active i {
  color: #b935f8;
  text-shadow: 0 0 10px rgba(185, 53, 248, 0.7);
}

.subtab-btn.active .subtab-glow {
  opacity: 1;
  background: radial-gradient(ellipse at center, rgba(0, 212, 255, 0.2) 0%, transparent 70%);
}

.subtab-btn.active::after {
  content: '';
  position: absolute;
  bottom: -1px;
  left: 50%;
  transform: translateX(-50%);
  width: 60%;
  height: 2px;
  background: linear-gradient(90deg, transparent, #00d4ff, transparent);
  box-shadow: 0 0 10px #00d4ff;
}

.subtab-indicator {
  position: absolute;
  bottom: 0;
  left: 0;
  height: 2px;
  background: linear-gradient(90deg, #b935f8, #00d4ff);
  box-shadow: 0 0 10px rgba(0, 212, 255, 0.5);
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  border-radius: 2px;
}

/* Sub-tab Content */
.subtab-content {
  display: none;
  opacity: 0;
  transform: translateY(10px);
  transition: opacity 0.3s ease, transform 0.3s ease;
}

.subtab-content.active {
  display: block;
  opacity: 1;
  transform: translateY(0);
  animation: subtabFadeIn 0.3s ease forwards;
}

@keyframes subtabFadeIn {
  from {
    opacity: 0;
    transform: translateY(10px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

/* Responsive adjustments */
@media (max-width: 768px) {
  .subtab-btn {
    padding: 0.6rem 1rem;
    font-size: 0.85rem;
  }

  .subtab-btn span {
    display: none;
  }

  .subtab-btn i {
    font-size: 1.2rem;
  }
}

  .detail-type {
    color: #c967f7;
    font-weight: bold;
  }

  .detail-item {
    display: flex;
    margin-bottom: 0.5rem;
  }


.tabulator .tabulator-tableholder .tabulator-table {
    background-color: #10051c !important;
}

  .detail-label {
    width: 200px;
    min-width: 200px;
    color: #b3b3cc;
    padding-right: 1rem;
  }

  .detail-value {
    flex: 1;
    word-break: break-all;

  }

  .hash-value {
    font-family: monospace;
    background-color: rgba(10, 11, 22, 0.5);
    padding: 2px 4px;
    border-radius: 4px;
  }

  /* GLink copy button */
  .glink-copy-btn {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    padding: 4px 10px;
    font-size: 0.75rem;
    color: #b935f8;
    background: rgba(185, 53, 248, 0.1);
    border: 1px solid rgba(185, 53, 248, 0.4);
    border-radius: 4px;
    cursor: pointer;
    transition: all 0.2s ease;
    font-family: inherit;
  }

  .glink-copy-btn:hover {
    background: rgba(185, 53, 248, 0.2);
    border-color: rgba(185, 53, 248, 0.7);
    box-shadow: 0 0 8px rgba(185, 53, 248, 0.3);
  }

  .glink-copy-btn:active {
    transform: scale(0.95);
  }

  .glink-copy-btn i {
    font-size: 0.7rem;
  }

  /* Share notification toast styling */
  .explorer-share-toast {
    background: linear-gradient(135deg, #1a0a2e 0%, #16213e 100%) !important;
    border: 1px solid rgba(185, 53, 248, 0.5) !important;
    box-shadow: 0 0 20px rgba(185, 53, 248, 0.3), inset 0 0 30px rgba(0, 212, 255, 0.05) !important;
  }

  .explorer-share-toast .swal2-title {
    font-family: 'Segoe UI', 'Roboto', 'Arial', sans-serif !important;
    font-size: 0.95rem !important;
    font-weight: 500 !important;
    color: #00d4ff !important;
    text-shadow: 0 0 10px rgba(0, 212, 255, 0.5) !important;
    letter-spacing: 0.5px !important;
  }

  .explorer-share-toast .swal2-icon.swal2-success {
    border-color: rgba(0, 212, 255, 0.4) !important;
  }

  .explorer-share-toast .swal2-icon.swal2-success [class^='swal2-success-line'] {
    background-color: #00d4ff !important;
  }

  .explorer-share-toast .swal2-icon.swal2-success .swal2-success-ring {
    border-color: rgba(0, 212, 255, 0.3) !important;
  }

  .explorer-share-toast .swal2-timer-progress-bar {
    background: linear-gradient(90deg, #b935f8, #00d4ff) !important;
  }

  /* Loading animation */
  .loading-overlay {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: rgba(5, 7, 20, 0.7);
    display: flex;
    justify-content: center;
    align-items: center;
    z-index: 1000;
    backdrop-filter: blur(3px);
  }
  
  .interactive-element {
  cursor: pointer;
  position: relative;
  padding: 2px 4px;
  transition: all 0.2s ease;
  border-radius: 3px;
}

.interactive-element:before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  border-radius: 3px;
  background: rgba(0, 230, 255, 0.05);
  box-shadow: 0 0 5px rgba(0, 230, 255, 0.2);
  opacity: 0;
  transition: opacity 0.3s ease;
  z-index: -1;
}

.interactive-element:hover, .interactive-element:focus {
  color: #00e6ff;
  text-shadow: 0 0 8px rgba(0, 230, 255, 0.7);
}

.interactive-element:hover:before, .interactive-element:focus:before {
  opacity: 1;
}

.interactive-element:after {
  content: '↗';
  font-family: monospace;
  margin-left: 4px;
  font-size: 0.8em;
  opacity: 0;
  transition: opacity 0.2s ease;
  position: relative;
  top: -2px;
  color: #00e6ff;
}

.interactive-element:hover:after, .interactive-element:focus:after {
  opacity: 1;
}

  .loading-spinner {
    width: 50px;
    height: 50px;
    border: 3px solid transparent;
    border-top: 3px solid #3d6eff;
    border-right: 3px solid #b935f8;
    border-bottom: 3px solid #0cffff;
    border-radius: 50%;
    animation: spin 1.5s linear infinite;
  }

  @keyframes spin {
    0% { transform: rotate(0deg); }
    100% { transform: rotate(360deg); }
  }

  /* Media Queries for Responsiveness */
  @media (max-width: 768px) {
    .main-content {
      flex-direction: column;
    }
    
    .sidebar {
      width: 100%;
      border-right: none;
      border-bottom: 1px solid rgba(59, 110, 255, 0.3);
      max-height: 50px;
      overflow: hidden;
    }
    
    .sidebar.sidebar-expanded {
      max-height: 300px;
    }
    
    .nav-items {
      display: flex;
      flex-direction: column;
    }
    
    .sidebar-toggle {
      display: block;
      text-align: center;
      padding: 0.5rem;
      background-color: #050714;
      cursor: pointer;
    }
    
    .stats-grid {
      grid-template-columns: 1fr;
    }
    
    .detail-item {
      flex-direction: column;
    }
    
    .detail-label {
      width: 100%;
      margin-bottom: 0.25rem;
    }
  }
  
  /* Auto Complete CSS - BEGIN*/
 
  /* Auto Compelte CSS - END*/

  /* Transition animations */
  .fade-in {
    animation: fadeIn 0.3s ease-in;
  }
  
  @keyframes fadeIn {
    from { opacity: 0; }
    to { opacity: 1; }
  }
  
  /* Custom scrollbar */
  ::-webkit-scrollbar {
    width: 8px;
    height: 8px;
  }
  
  ::-webkit-scrollbar-track {
    background: #050714;
  }
  
  ::-webkit-scrollbar-thumb {
    background: #3d6eff;
    border-radius: 4px;
  }
  
  ::-webkit-scrollbar-thumb:hover {
    background: #b935f8;
  }
  
   .pagination-container {
                display: flex;
                justify-content: space-between;
                align-items: center;
                margin-top: 1rem;
                padding: 0.5rem;
                border-radius: 4px;
                background: rgba(61, 110, 255, 0.1);
            }
            
            .pagination-info {
                font-size: 0.85rem;
                color: #b3b3cc;
            }
            
            .pagination-buttons {
                display: flex;
                gap: 0.25rem;
            }
            
            .pagination-button {
                min-width: 32px;
                height: 32px;
                background: rgba(61, 110, 255, 0.2);
                border: 1px solid rgba(59, 110, 255, 0.3);
                border-radius: 4px;
                color: #ffffff;
                cursor: pointer;
                display: flex;
                align-items: center;
                justify-content: center;
                transition: all 0.2s ease;
            }
            
            .pagination-button:hover:not([disabled]) {
                background: rgba(61, 110, 255, 0.4);
            }
            
            .pagination-button.active {
                background: #3d6eff;
                color: white;
            }
            
            .pagination-button[disabled] {
                opacity: 0.5;
                cursor: not-allowed;
            }
            
            .pagination-size {
                display: flex;
                align-items: center;
                gap: 0.5rem;
                font-size: 0.85rem;
            }
            
            .page-size-select {
                 background: rgb(2 8 26 / 87%);
                border: 1px solid rgba(59, 110, 255, 0.3);
                border-radius: 4px;
                  color: #e7f3f6;
                padding: 0.25rem 0.5rem;
            }
			
		/* Cyberpunk Sidebar Styles */
.sidebar {
  width: 250px;
  background-color: #0a0b16;
  border-right: 1px solid rgba(61, 110, 255, 0.3);
  overflow-y: auto;
  transition: width 0.3s ease;
  position: relative;
  z-index: 10;
}

.sidebar-inner {
  position: relative;
  height: 100%;
  overflow: hidden;
}

.circuit-pattern {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  opacity: 0.1;
  background-image: 
    linear-gradient(to right, #3d6eff 0.5px, transparent 1px),
    linear-gradient(to bottom, #3d6eff 0.5px, transparent 1px),
    radial-gradient(circle, #b935f8 1px, transparent 2px);
  background-size: 20px 20px, 20px 20px, 60px 60px;
  background-position: 0 0, 0 0, 10px 10px;
  pointer-events: none;
}

.nav-items {
  position: relative;
  z-index: 2;
  padding-top: 10px;
}

.nav-item {
  padding: 12px 18px;
  cursor: pointer;
  transition: all 0.3s ease;
  display: flex;
  align-items: center;
  position: relative;
  overflow: hidden;
  margin: 8px;
  border-radius: 4px;
  background-color: rgba(10, 11, 22, 0.7);
  border-left: 2px solid transparent;
}

.nav-item:hover {
  background-color: rgba(61, 110, 255, 0.15);
  transform: translateX(3px);
}

.nav-item.active {
  background-color: rgba(61, 110, 255, 0.25);
  border-left: 2px solid #b935f8;
}

.nav-item.active::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: linear-gradient(to right, rgba(185, 53, 248, 0.1), transparent);
  pointer-events: none;
}

.nav-item.active::after {
  content: '';
  position: absolute;
  top: -10px;
  right: -10px;
  width: 20px;
  height: 20px;
  border-radius: 50%;
  background: #b935f8;
  filter: blur(15px);
  opacity: 0.5;
  animation: pulse 2s infinite;
}

.nav-icon-wrapper {
  position: relative;
  width: 24px;
  height: 24px;
  margin-right: 12px;
}

.nav-icon {
  width: 100%;
  height: 100%;
  fill: #ffffff;
  transition: all 0.3s ease;
  position: relative;
  z-index: 1;
}

.nav-item:hover .nav-icon {
  fill: #0cffff;
  transform: scale(1.1);
}

.nav-item.active .nav-icon {
  fill: #0cff0c;
}

.icon-glow {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  border-radius: 50%;
  background: transparent;
  transition: all 0.3s ease;
  pointer-events: none;
}

.nav-item:hover .icon-glow {
  background: radial-gradient(circle, rgba(12, 255, 255, 0.3) 0%, transparent 70%);
  box-shadow: 0 0 8px 2px rgba(12, 255, 255, 0.4);
}

.nav-item.active .icon-glow {
  background: radial-gradient(circle, rgba(12, 255, 12, 0.3) 0%, transparent 70%);
  box-shadow: 0 0 8px 2px rgba(12, 255, 12, 0.4);
}

.nav-text {
  font-family: 'Orbitron', 'Arial', sans-serif;
  color: #ffffff;
  font-size: 14px;
  letter-spacing: 0.5px;
  transition: all 0.3s ease;
  text-shadow: 0 0 3px rgba(61, 110, 255, 0.5);
  position: relative;
  z-index: 1;
}

.nav-item:hover .nav-text {
  transform: translateX(3px);
  color: #0cffff;
}

.nav-item.active .nav-text {
  color: #0cff0c;
}

.nav-indicator {
  position: absolute;
  right: 10px;
  height: 0;
  width: 3px;
  background: #b935f8;
  transition: all 0.3s ease;
  opacity: 0;
}

.nav-item:hover .nav-indicator {
  height: 50%;
  opacity: 0.5;
}

.nav-item.active .nav-indicator {
  height: 80%;
  opacity: 1;
  box-shadow: 0 0 8px 2px rgba(185, 53, 248, 0.5);
}

.sidebar-collapsed {
  width: 60px;
}

.sidebar-collapsed .nav-text {
  display: none;
}

.sidebar-collapsed .nav-item {
  padding: 15px 0;
  justify-content: center;
}

.sidebar-collapsed .nav-icon-wrapper {
  margin-right: 0;
}

@keyframes pulse {
  0% {
    opacity: 0.5;
    transform: scale(1);
  }
  50% {
    opacity: 0.7;
    transform: scale(1.2);
  }
  100% {
    opacity: 0.5;
    transform: scale(1);
  }
}

/* Media query for responsive design */
@media (max-width: 768px) {
  .sidebar {
    width: 100%;
    border-right: none;
    border-bottom: 1px solid rgba(61, 110, 255, 0.3);
    max-height: 60px;
    overflow: hidden;
  }

  .sidebar.sidebar-expanded {
    max-height: 300px;
  }

  .nav-items {
    display: flex;
    flex-direction: column;
  }

  .sidebar-toggle {
    display: block;
    text-align: center;
    padding: 0.5rem;
    background-color: #0a0b16;
    cursor: pointer;
  }
}	

/* Code display styling */
.code-section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.code-toggle-btn {
  background: rgba(61, 110, 255, 0.3);
  border: 1px solid rgba(185, 53, 248, 0.5);
  border-radius: 4px;
  color: #ffffff;
  cursor: pointer;
  width: 28px;
  height: 28px;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all 0.3s ease;
  outline: none;
  box-shadow: 0 0 5px rgba(185, 53, 248, 0.3);
}

.code-toggle-btn:hover {
  background: rgba(185, 53, 248, 0.3);
  box-shadow: 0 0 8px rgba(185, 53, 248, 0.5);
}

.code-container {
  background: rgba(0, 0, 0, 0.5);
  border: 1px solid rgba(61, 110, 255, 0.3);
  border-radius: 4px;
  box-shadow: inset 0 0 10px rgba(12, 255, 255, 0.2);
  margin-top: 10px;
  position: relative;
  transition: all 0.3s ease;
  width: 50%;
}

.code-container:before {
  content: '';
  position: absolute;
  top: -2px;
  left: -2px;
  right: -2px;
  bottom: -2px;
  background: linear-gradient(45deg, rgba(185, 53, 248, 0.1), rgba(61, 110, 255, 0.1), rgba(12, 255, 255, 0.1));
  border-radius: 5px;
  z-index: -1;
  animation: borderGlow 3s infinite alternate;
}

@keyframes borderGlow {
  0% { opacity: 0.3; }
  100% { opacity: 0.8; }
}

.code-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background: rgba(0, 0, 0, 0.7);
  padding: 8px 15px;
  border-bottom: 1px solid rgba(61, 110, 255, 0.3);
  border-radius: 3px 3px 0 0;
}

.code-title {
  color: #0cffff;
  font-family: 'Courier New', monospace;
  text-transform: uppercase;
  letter-spacing: 1px;
  text-shadow: 0 0 5px rgba(12, 255, 255, 0.7);
}

.copy-btn {
  background: rgba(0, 0, 0, 0.5);
  border: 1px solid rgba(12, 255, 255, 0.3);
  border-radius: 3px;
  color: #0cffff;
  cursor: pointer;
  width: 30px;
  height: 30px;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all 0.3s ease;
}

.copy-btn:hover {
  background: rgba(12, 255, 255, 0.2);
  box-shadow: 0 0 8px rgba(12, 255, 255, 0.5);
}

.code-display {
  margin: 0;
  padding: 15px;
  overflow: auto;
  max-height: 300px;
  font-family: 'Courier New', monospace;
  line-height: 1.5;
  font-size: 0.9rem;
  background: transparent;
}

/* Syntax highlighting overrides - direct colors instead of variables */
.hljs {
  background: transparent;
  color: #e6e6e6;
}

.hljs-keyword {
  color: #ff00ff; /* Magenta */
}

.hljs-built_in {
  color: #0cffff; /* Cyan */
}

.hljs-string {
  color: #0cff0c; /* Green */
}

.hljs-number {
  color: #f4fb50; /* Yellow */
}

.hljs-function {
  color: #3d6eff; /* Blue */
}

.hljs-params {
  color: #b935f8; /* Purple */
}

.hljs-variable {
  color: #ff3860; /* Red */
}

.hljs-comment {
  color: #777777; /* Gray */
}

/* Log display styling */
.log-section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.log-toggle-btn {
  background: rgba(61, 110, 255, 0.3);
  border: 1px solid rgba(185, 53, 248, 0.5);
  border-radius: 4px;
  color: #ffffff;
  cursor: pointer;
  width: 28px;
  height: 28px;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all 0.3s ease;
  outline: none;
  box-shadow: 0 0 5px rgba(185, 53, 248, 0.3);
}

.log-toggle-btn:hover {
  background: rgba(185, 53, 248, 0.3);
  box-shadow: 0 0 8px rgba(185, 53, 248, 0.5);
}

.log-container {
  background: rgba(0, 0, 0, 0.5);
  border: 1px solid rgba(61, 110, 255, 0.3);
  border-radius: 4px;
  box-shadow: inset 0 0 10px rgba(12, 255, 255, 0.2);
  margin-top: 10px;
  position: relative;
  transition: all 0.3s ease;
}

.log-container:before {
  content: '';
  position: absolute;
  top: -2px;
  left: -2px;
  right: -2px;
  bottom: -2px;
  background: linear-gradient(45deg, rgba(12, 255, 255, 0.1), rgba(61, 110, 255, 0.1));
  border-radius: 5px;
  z-index: -1;
  animation: logGlow 4s infinite alternate;
}

@keyframes logGlow {
  0% { opacity: 0.2; }
  100% { opacity: 0.6; }
}

.log-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background: rgba(0, 0, 0, 0.7);
  padding: 8px 15px;
  border-bottom: 1px solid rgba(61, 110, 255, 0.3);
  border-radius: 3px 3px 0 0;
}

.log-title {
  color: #0cffff;
  font-family: 'Courier New', monospace;
  text-transform: uppercase;
  letter-spacing: 1px;
  text-shadow: 0 0 5px rgba(12, 255, 255, 0.7);
}

.log-display {
  margin: 0;
  padding: 15px;
  overflow: auto;
  max-height: 300px;
  font-family: 'Courier New', monospace;
  line-height: 1.5;
  font-size: 0.9rem;
  color: #e6e6e6;
  background: transparent;
}

.log-entry {
  position: relative;
  padding-left: 20px;
  margin-bottom: 8px;
  line-height: 1.4;
  min-height: 20px;
}

.log-entry:before {
  content: '>';
  position: absolute;
  left: 0;
  top: 0;
  color: #3d6eff;
  font-weight: bold;
}

.log-entry.success {
  color: #0cff0c;
}

.log-entry.error {
  color: #ff3860;
}

.log-entry.warning {
  color: #f4fb50;
}

.log-entry.info {
  color: #0cffff;
}

.log-entry.success:before {
  color: #0cff0c;
}

.log-entry.error:before {
  content: '!';
  color: #ff3860;
}

.log-entry.warning:before {
  content: '⚠';
  color: #f4fb50;
}

.log-timestamp {
  color: #777;
  font-size: 0.8em;
  margin-right: 8px;
}

.log-execution {
  position: relative;
  margin-top: 15px;
  padding-top: 10px;
  border-top: 1px dashed rgba(61, 110, 255, 0.3);
}

.log-execution:before {
  content: 'Execution';
  position: absolute;
  top: -8px;
  left: 10px;
  background: rgba(0, 0, 0, 0.7);
  padding: 0 10px;
  font-size: 0.8em;
  color: #b935f8;
}

.blink {
  animation: blink 1s infinite;
}

@keyframes blink {
  0%, 100% { opacity: 1; }
  50% { opacity: 0; }
}

@keyframes cyber-collapse {
  0% {
    opacity: 1;
    transform: scaleY(1) translateY(0);
    box-shadow: 0 0 15px var(--neon-blue);
    clip-path: polygon(0 0, 100% 0, 100% 100%, 0 100%);
  }
  20% {
    opacity: 0.9;
    clip-path: polygon(0 0, 100% 0, 98% 100%, 2% 100%);
    box-shadow: 0 0 20px var(--neon-purple);
    text-shadow: 2px 2px 5px var(--neon-purple);
  }
  40% {
    opacity: 0.7;
    transform: scaleY(0.6) translateY(-20px);
    clip-path: polygon(5% 0, 95% 0, 90% 100%, 10% 100%);
    box-shadow: 0 0 10px var(--neon-green);
  }
  60% {
    opacity: 0.5;
    text-shadow: -3px 0 8px var(--neon-green);
    background-image: linear-gradient(90deg, rgba(10, 11, 22, 0.7) 0%, rgba(61, 110, 255, 0.2) 50%, rgba(10, 11, 22, 0.7) 100%);
  }
  80% {
    opacity: 0.2;
    clip-path: polygon(25% 0, 75% 0, 80% 50%, 20% 50%);
    box-shadow: 0 0 25px var(--neon-cyan);
  }
  100% {
    opacity: 0;
    transform: scaleY(0) translateY(-50px);
    height: 0;
    box-shadow: 0 0 30px var(--neon-pink);
    clip-path: polygon(50% 0, 50% 0, 50% 0, 50% 0);
  }
}

@keyframes cyber-expand {
  0% {
    opacity: 0;
    transform: scaleY(0) translateY(-50px);
    height: auto;
    box-shadow: 0 0 30px var(--neon-pink);
    clip-path: polygon(50% 0, 50% 0, 50% 0, 50% 0);
    filter: blur(5px);
  }
  20% {
    opacity: 0.2;
    filter: blur(4px);
    clip-path: polygon(25% 0, 75% 0, 80% 50%, 20% 50%);
    box-shadow: 0 0 25px var(--neon-cyan);
  }
  40% {
    opacity: 0.4;
    background-image: linear-gradient(90deg, rgba(10, 11, 22, 0.7) 0%, rgba(61, 110, 255, 0.2) 50%, rgba(10, 11, 22, 0.7) 100%);
    text-shadow: 0 0 8px var(--neon-green);
  }
  60% {
    opacity: 0.6;
    transform: scaleY(0.6) translateY(-20px);
    filter: blur(2px);
    clip-path: polygon(5% 0, 95% 0, 90% 100%, 10% 100%);
    box-shadow: 0 0 10px var(--neon-green);
  }
  80% {
    opacity: 0.8;
    clip-path: polygon(0 0, 100% 0, 98% 100%, 2% 100%);
    box-shadow: 0 0 20px var(--neon-purple);
    text-shadow: 1px 1px 3px var(--neon-purple);
  }
  100% {
    opacity: 1;
    transform: scaleY(1) translateY(0);
    filter: blur(0);
    box-shadow: 0 0 15px var(--neon-blue);
    clip-path: polygon(0 0, 100% 0, 100% 100%, 0 100%);
  }
}

.cyber-collapsing {
  animation: cyber-collapse 0.8s forwards;
  transform-origin: top;
  overflow: hidden;
  position: relative;
}

.cyber-expanding {
  animation: cyber-expand 0.8s forwards;
  transform-origin: top;
  overflow: hidden;
  position: relative;
}

/* Digital noise effect overlay */
.cyber-glitch::before {
  content: "";
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background-image: url("data:image/svg+xml,%3Csvg viewBox='0 0 250 250' xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='noiseFilter'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.8' numOctaves='3' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23noiseFilter)'/%3E%3C/svg%3E");
  opacity: 0.2;
  pointer-events: none;
  z-index: 1;
  mix-blend-mode: overlay;
}

@keyframes flicker-text {
  0%, 100% { opacity: 1; }
  10% { opacity: 0.8; }
  20% { opacity: 1; }
  30% { opacity: 0.6; }
  40% { opacity: 1; }
  50% { opacity: 0.9; }
  60% { opacity: 1; }
  70% { opacity: 0.7; }
  80% { opacity: 1; }
  90% { opacity: 0.5; }
}

.cyber-text {
  animation: flicker-text 1.5s infinite;
  text-shadow: 0 0 5px var(--neon-blue);
}

.holo-header {
  position: relative;
  height: 4rem;
  overflow: hidden;
  display: flex;
  align-items: center;
  background-color: rgba(0, 0, 20, 0.8);
  border-radius: 8px;
  box-shadow: 
    inset 0 0 30px rgba(61, 110, 255, 0.5),
    0 0 15px rgba(61, 110, 255, 0.3);
}

.holo-scanlines {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: linear-gradient(
    to bottom,
    transparent 50%,
    rgba(12, 255, 255, 0.05) 50%
  );
  background-size: 100% 4px;
  z-index: 10;
  pointer-events: none;
}

.holo-beams {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: 1;
  overflow: hidden;
}

.holo-beams::before, .holo-beams::after {
  content: '';
  position: absolute;
  top: -50%;
  left: -50%;
  right: -50%;
  bottom: -50%;
  background: 
    conic-gradient(
      transparent, 
      rgba(61, 110, 255, 0.1), 
      transparent 30%
    );
  animation: rotate 20s linear infinite;
}

.holo-beams::after {
  background: 
    conic-gradient(
      transparent, 
      rgba(12, 255, 255, 0.1), 
      transparent 20%
    );
  animation: rotate 15s linear infinite reverse;
}

.holo-projection {
  flex: 1;
  z-index: 5;
  padding-left: 2rem;
  position: relative;
}

.holo-projection::before {
  content: '';
  position: absolute;
  top: -20%;
  left: 1rem;
  width: 1px;
  height: 140%;
  background: linear-gradient(to bottom, 
    transparent, 
    #3d6eff, 
    #3d6eff, 
    transparent
  );
}

.holo-title {
  display: flex;
  align-items: center;
  gap: 15px;
  margin: 0;
}

.holo-icon {
  width: 2.5rem;
  height: 2.5rem;
  border-radius: 50%;
  background: 
    radial-gradient(
      circle at center,
      #0cffff 5%,
      rgba(12, 255, 255, 0.5) 30%,
      transparent 70%
    );
  position: relative;
  animation: pulse-glow 3s infinite;
}

.holo-icon::before, .holo-icon::after {
  content: '';
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  border-radius: 50%;
  background-color: rgba(12, 255, 255, 0.2);
}

.holo-icon::before {
  width: 80%;
  height: 80%;
  animation: pulse-glow 3s infinite 0.5s;
}

.holo-icon::after {
  width: 120%;
  height: 120%;
  animation: pulse-glow 3s infinite 1s;
}

.holo-text {
  display: flex;
  flex-direction: column;
}

.holo-main {
  font-size: 1.8rem;
  font-weight: 800;
  letter-spacing: 3px;
  color: #3d6eff;
  text-shadow: 0 0 10px #3d6eff;
  position: relative;
  transform-style: preserve-3d;
  animation: float 6s ease-in-out infinite;
}

.holo-main::before {
  content: 'GRIDNET';
  position: absolute;
  left: 0;
  top: 0;
  color: #0cffff;
  opacity: 0.5;
  filter: blur(4px);
  animation: holo-shift 4s infinite;
}

.holo-sub {
  font-size: 0.8rem;
  font-weight: normal;
  letter-spacing: 2px;
  color: #a0a0b0;
  opacity: 0.8;
  margin-top: -5px;
}

.holo-interface {
  display: flex;
  flex-direction: column;
  gap: 5px;
  margin-right: 2rem;
  z-index: 5;
  width: 180px;
}

.holo-meter {
  display: flex;
  align-items: center;
  gap: 10px;
}

.meter-label {
  font-size: 0.7rem;
  color: #a0a0b0;
  width: 60px;
  text-align: right;
}

.meter-bar {
  flex: 1;
  height: 4px;
  background-color: rgba(255, 255, 255, 0.1);
  border-radius: 2px;
  overflow: hidden;
  position: relative;
}

.meter-fill {
  position: absolute;
  top: 0;
  left: 0;
  height: 100%;
  background: linear-gradient(90deg, #3d6eff, #0cffff);
  width: 85%;
  box-shadow: 0 0 8px #0cffff;
  animation: meter-fluctuate 8s infinite;
}

.holo-meter:nth-child(2) .meter-fill {
  width: 92%;
  animation-delay: 2s;
  animation-direction: alternate-reverse;
}

.holo-data {
  display: flex;
  justify-content: space-between;
  font-size: 0.7rem;
  color: #0cffff;
  margin-top: 5px;
}

.data-bit {
  text-shadow: 0 0 5px #0cffff;
}

.blink-slow {
  animation: blink-slow 4s infinite;
}

#header-block-height {
  font-weight: bold;
}

@keyframes rotate {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

@keyframes pulse-glow {
  0%, 100% { opacity: 0.5; box-shadow: 0 0 10px #0cffff; }
  50% { opacity: 1; box-shadow: 0 0 20px #0cffff, 0 0 30px rgba(12, 255, 255, 0.4); }
}

@keyframes float {
  0%, 100% { transform: translateY(0) translateZ(0); }
  50% { transform: translateY(-3px) translateZ(10px); }
}

@keyframes holo-shift {
  0%, 100% { left: -2px; top: -2px; }
  25% { left: 2px; top: -1px; }
  50% { left: 1px; top: 2px; }
  75% { left: -1px; top: 1px; }
}

@keyframes meter-fluctuate {
  0%, 100% { width: 85%; }
  25% { width: 87%; }
  50% { width: 83%; }
  75% { width: 88%; }
}

@keyframes blink-slow {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.3; }
}

/* Market Info Search */
.domain-controls {
  display: flex;
  flex-direction: column;
  gap: 1rem;
  margin-bottom: 1rem;
}

.market-controls {
  display: flex;
  flex-wrap: wrap;
  gap: 0.8rem;
  align-items: center;
  background-color: rgba(10, 11, 22, 0.7);
  border: 1px solid rgba(61, 110, 255, 0.3);
  border-radius: 8px;
  padding: 0.8rem 1rem;
  box-shadow: 0 0 10px rgba(10, 11, 22, 0.5);
}

.control-group {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.control-group label {
  color: #b3b3cc;
  font-size: 0.9rem;
}

.market-control {
  background-color: rgba(10, 11, 22, 0.7);
  border: 1px solid rgba(61, 110, 255, 0.3);
  border-radius: 4px;
  color: #ffffff;
  padding: 0.4rem 0.6rem;
  font-family: inherit;
  min-width: 120px;
}

.market-control:focus {
  outline: none;
  border-color: #b935f8;
  box-shadow: 0 0 5px rgba(185, 53, 248, 0.7);
}

.market-button {
  background: linear-gradient(to right, #3d6eff, #b935f8);
  border: none;
  border-radius: 4px;
  color: white;
  padding: 0.5rem 1rem;
  cursor: pointer;
  font-weight: bold;
  transition: all 0.3s ease;
  margin-left: auto;
}

.market-button:hover {
  box-shadow: 0 0 5px rgba(185, 53, 248, 0.7);
  transform: translateY(-1px);
}

</style>

<div class="blockchain-explorer">
  <!-- Header -->
  <div class="header">
    <div id="header-title" class="header-title">
      <div class="header-title holo-header">
  <div class="holo-scanlines"></div>
  <div class="holo-beams"></div>
  <div class="holo-projection">
    <h1 class="holo-title">
      <div class="holo-icon"></div>
      <div class="holo-text">
        <span class="holo-main">GRIDNET</span>
        <span class="holo-sub">BLOCKCHAIN EXPLORER</span>
      </div>
    </h1>
  </div>
  <div class="holo-interface">
    <div class="holo-meter">
      <div class="meter-label">NETWORK</div>
      <div class="meter-bar">
        <div class="meter-fill"></div>
      </div>
    </div>
    <div class="holo-meter">
      <div class="meter-label">SECURITY</div>
      <div class="meter-bar">
        <div class="meter-fill"></div>
      </div>
    </div>
    <div class="holo-data">
      <span class="data-bit">BLOCK: <span id="header-block-height">0</span></span>
      <span class="data-bit blink-slow">ACTIVE</span>
    </div>
  </div>
</div>
    </div>
    <div class="header-status">
      <div class="status-dot status-live" id="liveness-indicator"></div>
      <span id="blockchain-status">Network: Online</span>
      <span style="margin-left: 15px;">Height: <span id="blockchain-height">0</span></span>
      <span style="margin-left: 15px;">USDT: $<span id="usdt-price">0.00</span></span>
	  <span style="margin-left: 15px;">Market: <span id="market-cap">pending</span></span>
	  
    </div>
  </div>
  
  <!-- Search Section -->
  <div class="search-section">
    <input type="text" class="search-input" id="search-input" placeholder="Search by Block ID, Transaction ID, or Domain...">
    <button class="search-button" id="search-button">Search</button>
  </div>
  
  <!-- Main Content -->
  <div class="main-content">
    <!-- Sidebar -->
 <div class="sidebar" id="sidebar">
  <div class="sidebar-inner">
    <div class="circuit-pattern"></div>
    <div class="nav-items">
      <div class="nav-item active" data-section="dashboard">
        <div class="nav-icon-wrapper">
          <svg class="nav-icon" viewBox="0 0 24 24">
            <path d="M3 3h8v8H3V3zm10 0h8v8h-8V3zm0 10h8v8h-8v-8zM3 13h8v8H3v-8z"/>
          </svg>
          <div class="icon-glow"></div>
        </div>
        <span class="nav-text">Dashboard</span>
        <div class="nav-indicator"></div>
      </div>
      <div class="nav-item" data-section="blocks">
        <div class="nav-icon-wrapper">
          <svg class="nav-icon" viewBox="0 0 24 24">
            <path d="M12 2L2 7v10l10 5 10-5V7L12 2zm0 18l-8-4V8l8 4 8-4v8l-8 4zm0-12L4 8l8-4 8 4-8 4z"/>
          </svg>
          <div class="icon-glow"></div>
        </div>
        <span class="nav-text">Blocks</span>
        <div class="nav-indicator"></div>
      </div>
      <div class="nav-item" data-section="transactions">
        <div class="nav-icon-wrapper">
          <svg class="nav-icon" viewBox="0 0 24 24">
            <path d="M5 10v4h14v-4H5zm0-8h14v4H5V2zM5 20h14v-4H5v4z"/>
          </svg>
          <div class="icon-glow"></div>
        </div>
        <span class="nav-text">Transactions</span>
        <div class="nav-indicator"></div>
      </div>
      <div class="nav-item" data-section="domains">
        <div class="nav-icon-wrapper">
          <svg class="nav-icon" viewBox="0 0 24 24">
            <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-1 17.93c-3.95-.49-7-3.85-7-7.93 0-.62.08-1.21.21-1.79L9 15v1c0 1.1.9 2 2 2v1.93zm6.9-2.54c-.26-.81-1-1.39-1.9-1.39h-1v-3c0-.55-.45-1-1-1H8v-2h2c.55 0 1-.45 1-1V7h2c1.1 0 2-.9 2-2v-.41c2.93 1.19 5 4.06 5 7.41 0 2.08-.8 3.97-2.1 5.39z"/>
          </svg>
          <div class="icon-glow"></div>
        </div>
        <span class="nav-text">Domains</span>
        <div class="nav-indicator"></div>
      </div>
      <div class="nav-item" data-section="statistics">
        <div class="nav-icon-wrapper">
          <svg class="nav-icon" viewBox="0 0 24 24">
            <path d="M3 13h2v7H3v-7zm4-7h2v14H7V6zm4 3h2v11h-2V9zm4-3h2v14h-2V6zm4 6h2v8h-2v-8z"/>
          </svg>
          <div class="icon-glow"></div>
        </div>
        <span class="nav-text">Statistics</span>
        <div class="nav-indicator"></div>
      </div>
    </div>
  </div>
</div>
    
    <!-- Content Area -->
    <div class="content-area">
      <!-- Dashboard Section -->
      <div class="content-section active" id="dashboard-section">
   
		
		<div class="cyberpunk-header terminal-header">
  <div class="terminal-prefix">root@gridnet:~#</div>
  <h2 class="terminal-text">Network Overview</h2>
  <div class="terminal-cursor"></div>
</div>
        
        <div class="stats-grid">
          <div class="stat-panel">
            <div class="stat-title">Network Utilization</div>
            <div class="stat-value" id="network-utilization">0%</div>
          </div>
          <div class="stat-panel">
            <div class="stat-title">Average Block Size</div>
            <div class="stat-value" id="avg-block-size">0 KB</div>
          </div>
          <div class="stat-panel">
            <div class="stat-title">Block Rewards (24h)</div>
            <div class="stat-value" id="block-rewards">0 GNC</div>
          </div>
          <div class="stat-panel">
            <div class="stat-title">Avg Block Time</div>
            <div class="stat-value" id="avg-block-time">0 sec</div>
          </div>
        </div>
        
        <div class="chart-container">
          <div id="transactions-chart" style="width:100%; height:100%;"></div>
        </div>
        

		
	<div class="cyberpunk-header hologram-header">
  <h2 class="hologram-text">Latest Blocks</h2>
  <div class="hologram-light"></div>
  <div class="hologram-flicker"></div>
</div>
        <div class="data-table" id="recent-blocks-table"></div>
        
		
			<div class="cyberpunk-header hologram-header">
  <h2 class="hologram-text">Latest Transactions</h2>
  <div class="hologram-light"></div>
  <div class="hologram-flicker"></div>
</div>

        <div class="data-table" id="recent-transactions-table"></div>
      </div>
      
      <!-- Blocks Section -->
      <div class="content-section" id="blocks-section">
        <div class="cyberpunk-header terminal-header">
  <div class="terminal-prefix">root@gridnet:~#</div>
  <h2 class="terminal-text">Blockchain Blocks</h2>
  <div class="terminal-cursor"></div>
</div>

        <!-- Horizontal Sub-tabs for Blocks View -->
        <div class="blocks-subtabs">
          <div class="subtab-nav">
            <button class="subtab-btn active" data-subtab="blocks-list">
              <i class="fa fa-cubes"></i>
              <span>Blocks</span>
              <div class="subtab-glow"></div>
            </button>
            <button class="subtab-btn" data-subtab="block-details-view">
              <i class="fa fa-cube"></i>
              <span>Block Details</span>
              <div class="subtab-glow"></div>
            </button>
          </div>
          <div class="subtab-indicator"></div>
        </div>

        <!-- Blocks List Tab Content -->
        <div class="subtab-content active" id="blocks-list">
          <div class="data-table" id="blocks-table"></div>
        </div>

        <!-- Block Details Tab Content -->
        <div class="subtab-content" id="block-details-view">
          <div id="block-details"></div>
        </div>
      </div>
      
      <!-- Transactions Section -->
      <div class="content-section" id="transactions-section">
       <div class="cyberpunk-header terminal-header">
  <div class="terminal-prefix">root@gridnet:~#</div>
  <h2 class="terminal-text">Transactions</h2>
  <div class="terminal-cursor"></div>
</div>
		<div id="transaction-details"></div>
        <div class="data-table" id="transactions-table"></div>
       
      </div>
      
      <!-- Domains Section -->
      <div class="content-section" id="domains-section">
        <div class="cyberpunk-header terminal-header">
  <div class="terminal-prefix">root@gridnet:~#</div>
  <h2 class="terminal-text">Domain Explorer</h2>
  <div class="terminal-cursor"></div>
</div>
        <div class="domain-search">
          <input type="text" class="search-input" id="domain-search-input" placeholder="Enter domain address...">
             <button class="search-button" id="domain-search-button">Search</button>
    </div>
    
    <div class="market-controls">
      <div class="control-group">
        <label>Sort By:</label>
        <select id="market-sort-type" class="market-control">
          <option value="1">Balance</option>
          <option value="2">Total Received</option>
          <option value="3">Total Sent</option>
          <option value="4">Outgoing Tx Count</option>
          <option value="5">Incoming Tx Count</option>
		  <option value="6">Total Tx Count</option>
          <option value="7">Locked Assets</option>
          <option value="8">Mined</option>
		  
        </select>
      </div>
      <div class="control-group">
        <label>Order:</label>
        <select id="market-sort-order" class="market-control">
          <option value="1" selected>Descending</option>
          <option value="0">Ascending</option>
        </select>
      </div>
      <button class="market-button" id="market-data-button">View Top Domains</button>
    </div>
	  <div id="domain-details"></div>
        <div class="data-table" id="domain-history-table"></div>
  </div>
  

 
      
      <!-- Statistics Section -->
      <div class="content-section" id="statistics-section">
       
		<div class="cyberpunk-header terminal-header">
  <div class="terminal-prefix">root@gridnet:~#</div>
  <h2 class="terminal-text">Blockchain Statistics</h2>
  <div class="terminal-cursor"></div>
</div>
        
        <div class="chart-container">
          <div id="daily-stats-chart" style="width:100%; height:100%;"></div>
        </div>
        
        <div class="stats-grid">
          <div class="stat-panel">
            <div class="stat-title">Key Block Time (24h)</div>
            <div class="stat-value" id="key-block-time">0 sec</div>
          </div>
          <div class="stat-panel">
            <div class="stat-title">Network Utilization (24h)</div>
            <div class="stat-value" id="network-util-24h">0%</div>
          </div>
          <div class="stat-panel">
            <div class="stat-title">Block Rewards (24h)</div>
            <div class="stat-value" id="block-rewards-24h">0 GNC</div>
          </div>
          <div class="stat-panel">
            <div class="stat-title">Average Block Size (24h)</div>
            <div class="stat-value" id="block-size-24h">0 KB</div>
          </div>
        </div>
      </div>
      
      <!-- Loading Overlay -->
      <div class="loading-overlay" id="loading-overlay">
        <div class="loading-spinner"></div>
      </div>
    </div>
  </div>
</div>`;
//Window Body - END

// Import blockchain entity classes
import {
    CBlockDesc
}
from '/lib/BlockDesc.js'
import {
    CTransactionDesc
}
from '/lib/TransactionDesc.js'
import {
    CDomainDesc
}
from '/lib/DomainDesc.js'
import {
    CSearchResults
}
from '/lib/SearchResults.js'
import {
    COperatorSecurityInfo
}
from '/lib/OperatorSecurityInfo.js'

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
            KEY_HEIGHT: 17,
            MARKET_DATA: 18,
            MARKET_CAP: 19
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
        // Pass data and dataType to parent CWindow class for GLink support
        super(positionX, positionY, width, height, blockchainExplorerBody, "Experimental Blockchain Explorer (EBE)", CUIBlockchainExplorer.getIcon(), true, data, dataType, filePath, thread);

        // Initialize tools
        this.mTools = CTools.getInstance();

        // Initialize system or fallback enums
        this.enums = EnumInitializer.initialize();

        // Initialize metadata parser
        this.mMetaParser = new CVMMetaParser();

        // Initialize data manager with callbacks for updates and errors
        this.dataManager = new BlockchainDataManager(
                this,
                this.enums,
                this.handleDataUpdate.bind(this),
                this.handleError.bind(this));

        // Track window state
        this.mLastHeightRearangedAt = 0;
        this.mLastWidthRearangedAt = 0;
        this.mErrorMsg = "";
        this.initialized = false;
        this.subscriptionActive = false;

        // Set up controller thread
        this.mControllerThreadInterval = 30000; // 10 seconds
        this.mControlerExecuting = false;
        this.mControler = 0;

        // Pagination state
        this.paginationState = {
            blocks: {
                currentPage: 1,
                pageSize: 20,
                totalPages: 1,
                totalItems: 0,
                sortBy: this.enums.eSortBlocksBy.timestampDesc
            },
            transactions: {
                currentPage: 1,
                pageSize: 20,
                totalPages: 1,
                totalItems: 0,
                sortBy: this.enums.eSortTransactionsBy.timestampDesc
            },
            domainHistory: {
                currentPage: 1,
                pageSize: 20,
                totalPages: 1,
                totalItems: 0,
                sortBy: this.enums.eSortTransactionsBy.timestampDesc
            },
            search: {
                currentPage: 1,
                pageSize: 10,
                totalPages: 1,
                totalItems: 0,
                lastQuery: '',
                lastFilter: null
            }
        };
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

        return `data:image/png;base64,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`;
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

            // Apply settings to pagination state
            if (settings.defaultPageSize) {
                this.paginationState.blocks.pageSize = settings.defaultPageSize;
                this.paginationState.transactions.pageSize = settings.defaultPageSize;
                this.paginationState.domainHistory.pageSize = settings.defaultPageSize;
                this.paginationState.search.pageSize = settings.defaultPageSize;
            }
        } else
            return false;

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
            defaultPageSize: 50,
            showAdvancedStats: false
        };
        return new CAppSettings(CUIBlockchainExplorer.getPackageID(), obj);
    }

    /**
     * Handle search configuration changes
     * @param {Object} config - Search configuration
     */
    onSearchConfigChange(config) {
        // Store the current search configuration
        this.currentSearchConfig = config;
    }

    VMStateChangedCallback(event) {
        //let the back-end know about the current Terminal's dimensions

        if (event.extended) { //we're now interested only in the new VM status signaling mechanics and signaling targeting this very UI application.
            //the full-node set the process ID once the DTI got initialized through CVMContext.initDTI()
            //no need for keeping track of requests' IDs etc. full node is now fully DTI/UI dApp process-aware.
            if (event.processID != this.getProcessID) {
                return; //not for this instance
            }
        } else {
            return;
        }

        switch (event.state) {

        case eVMState.limitReached:

            break;
        case eVMState.initializing:

            break;
        case eVMState.ready:
            this.mThreadActive = true;

            break;
        case eVMState.aboutToShutDown:

            break;
        case eVMState.disabled:
            this.mThreadActive = false;
            break;
        default:

        }
    }

    /**
     * Initialize UI element references
     */
    initUIElements() {

        // Get references to DOM elements using consistent CWindow access methods
        this.elements = {
            marketCap: this.getControl('market-cap'),
            loadingOverlay: this.getControl('loading-overlay'),
            sidebar: this.getControl('sidebar'),
            navItems: $(this.getBody).find('.nav-item').toArray(),
            contentSections: $(this.getBody).find('.content-section').toArray(),
            searchInput: this.getControl('search-input'),
            searchButton: this.getControl('search-button'),
            domainSearchInput: this.getControl('domain-search-input'),
            domainSearchButton: this.getControl('domain-search-button'),
            marketSortType: this.getControl("market-sort-type"),
            marketSortOrder: this.getControl("market-sort-order"),
            marketDataButton: this.getControl("market-data-button"),
            blockchainStatus: this.getControl('blockchain-status'),
            blockchainHeight: this.getControl('blockchain-height'),
            blockchainHeaderHeight: this.getControl('header-block-height'),
            usdtPrice: this.getControl('usdt-price'),
            livenessIndicator: this.getControl('liveness-indicator'),
            networkUtilization: this.getControl('network-utilization'),
            avgBlockSize: this.getControl('avg-block-size'),
            blockRewards: this.getControl('block-rewards'),
            avgBlockTime: this.getControl('avg-block-time'),
            keyBlockTime: this.getControl('key-block-time'),
            networkUtil24h: this.getControl('network-util-24h'),
            blockRewards24h: this.getControl('block-rewards-24h'),
            blockSize24h: this.getControl('block-size-24h'),
            // Pagination elements - will be created dynamically if not existing
            paginationControls: {
                blocks: this.getControl('blocks-pagination'),
                transactions: this.getControl('transactions-pagination'),
                domainHistory: this.getControl('domain-history-pagination'),
                search: this.getControl('search-pagination')
            }
        };

        // Check if essential elements exist
        if (!this.elements || !this.elements.loadingOverlay) {
            console.error("Essential UI elements not found. Initialization failed.");
            return;
        }
    }
    // AutoComplete - BEGIN
    /**
     * Initialize custom autocomplete for search inputs
     */
    initAutoComplete() {
        // Get search inputs
        const searchInput = this.elements.searchInput;
        const domainSearchInput = this.elements.domainSearchInput;

        if (!searchInput && !domainSearchInput)
            return;

        // Disable browser's native autocomplete for both inputs
        [searchInput, domainSearchInput].filter(Boolean).forEach(input => {
            input.setAttribute("autocomplete", "off");
            input.setAttribute("autocorrect", "off");
            input.setAttribute("autocapitalize", "off");
            input.setAttribute("spellcheck", "false");
        });

        // Load recent searches from settings
        this.recentSearches = this.loadRecentSearches();

        // Setup autocomplete for each input
        if (searchInput)
            this.setupAutoCompleteForInput(searchInput, 'search');
        if (domainSearchInput)
            this.setupAutoCompleteForInput(domainSearchInput, 'domain');
    }

    /**
     * Set up autocomplete for a specific input
     * @param {HTMLElement} input The input element
     * @param {string} type The input type ('search' or 'domain')
     */
    setupAutoCompleteForInput(input, type) {
        // Create autocomplete container
        const autoCompleteContainer = document.createElement('div');
        autoCompleteContainer.className = 'autocomplete-container';
        autoCompleteContainer.style.display = 'none';

        // Style container with cyberpunk look
        Object.assign(autoCompleteContainer.style, {
            position: 'absolute',
            zIndex: '1000',
            width: '100%',
            maxHeight: '200px',
            overflowY: 'auto',
            backgroundColor: '#0a0b16',
            border: '1px solid #3d6eff',
            borderTop: 'none',
            boxShadow: '0 4px 15px rgba(12, 255, 255, 0.3)',
            borderRadius: '0 0 8px 8px'
        });

        // Insert container after input
        input.parentNode.insertBefore(autoCompleteContainer, input.nextSibling);

        // Store reference to container
        this[`${type}AutoCompleteContainer`] = autoCompleteContainer;

        // Focus event - show dropdown
        const focusHandler = () => {
            this.updateAutoComplete(input, type);
        };

        // Input event - update dropdown
        const inputHandler = () => {
            this.updateAutoComplete(input, type);
        };

        // Blur event - hide dropdown (with delay to allow clicks on options)
        const blurHandler = () => {
            setTimeout(() => {
                autoCompleteContainer.style.display = 'none';
            }, 200);
        };

        // Keydown event - navigation and selection
        const keydownHandler = (e) => {
            const container = autoCompleteContainer;
            const items = container.querySelectorAll('.autocomplete-item');
            if (!items.length)
                return;

            const activeItem = container.querySelector('.active');
            let activeIndex = Array.from(items).findIndex(item => item === activeItem);

            switch (e.key) {
            case 'ArrowDown':
                e.preventDefault();
                activeIndex = (activeIndex + 1) % items.length;
                this.setActiveAutoCompleteItem(items, activeIndex);
                break;

            case 'ArrowUp':
                e.preventDefault();
                activeIndex = (activeIndex - 1 + items.length) % items.length;
                this.setActiveAutoCompleteItem(items, activeIndex);
                break;

            case 'Enter':
                if (activeItem) {
                    e.preventDefault();
                    const value = activeItem.getAttribute('data-value');
                    input.value = value;
                    this.addRecentSearch(value);
                    container.style.display = 'none';

                    // Trigger search
                    if (type === 'search' && this.callbacks?.onSearch) {
                        this.callbacks.onSearch(value);
                    } else if (type === 'domain' && this.callbacks?.onDomainSearch) {
                        this.callbacks.onDomainSearch(value);
                    }
                }
                break;

            case 'Escape':
                container.style.display = 'none';
                break;
            }
        };

        // Add event listeners
        input.addEventListener('focus', focusHandler);
        input.addEventListener('input', inputHandler);
        input.addEventListener('blur', blurHandler);
        input.addEventListener('keydown', keydownHandler);

        // Store event listeners for cleanup
        this[`${type}AutoCompleteHandlers`] = {
            focus: focusHandler,
            input: inputHandler,
            blur: blurHandler,
            keydown: keydownHandler
        };
    }

    /**
     * Update autocomplete dropdown for an input
     * @param {HTMLElement} input The input element
     * @param {string} type The input type
     */
    updateAutoComplete(input, type) {
        const container = this[`${type}AutoCompleteContainer`];
        if (!container)
            return;

        const value = input.value.trim().toLowerCase();

        // Filter recent searches
        let filteredSearches = this.recentSearches.filter(search =>
                search.toLowerCase().includes(value)).slice(0, 5);

        // Show/hide container based on results
        if (filteredSearches.length === 0) {
            container.style.display = 'none';
            return;
        }

        // Clear container
        container.innerHTML = '';

        // Add items to container
        filteredSearches.forEach(search => {
            const item = document.createElement('div');
            item.className = 'autocomplete-item';
            item.setAttribute('data-value', search);

            // Style item with cyberpunk look
            Object.assign(item.style, {
                padding: '10px 15px',
                cursor: 'pointer',
                borderBottom: '1px solid rgba(61, 110, 255, 0.2)',
                transition: 'all 0.2s ease'
            });

            // Add neon highlight effect on hover
            item.addEventListener('mouseenter', () => {
                Object.assign(item.style, {
                    backgroundColor: 'rgba(61, 110, 255, 0.1)',
                    boxShadow: 'inset 0 0 10px rgba(12, 255, 255, 0.2)'
                });
            });

            item.addEventListener('mouseleave', () => {
                if (!item.classList.contains('active')) {
                    Object.assign(item.style, {
                        backgroundColor: 'transparent',
                        boxShadow: 'none'
                    });
                }
            });

            // Highlight matching text
            if (value) {
                const regex = new RegExp(`(${this.escapeRegExp(value)})`, 'gi');
                const html = search.replace(regex, '<span style="color: #b935f8; font-weight: bold;">$1</span>');
                item.innerHTML = html;
            } else {
                item.textContent = search;
            }

            // Click event
            item.addEventListener('click', () => {
                input.value = search;
                this.addRecentSearch(search);
                container.style.display = 'none';

                // Trigger search
                if (type === 'search' && this.callbacks?.onSearch) {
                    this.callbacks.onSearch(search);
                } else if (type === 'domain' && this.callbacks?.onDomainSearch) {
                    this.callbacks.onDomainSearch(search);
                }
            });

            container.appendChild(item);
        });

        // Position container below input
        const inputRect = input.getBoundingClientRect();
        const containerRect = this.mInstance.getBody.getBoundingClientRect();

        Object.assign(container.style, {
            top: `${input.offsetHeight}px`,
            display: 'block'
        });
    }

    /**
     * Set active autocomplete item
     * @param {NodeList} items List of autocomplete items
     * @param {number} index Index of the item to activate
     */
    setActiveAutoCompleteItem(items, index) {
        // Remove active class from all items
        items.forEach(item => {
            item.classList.remove('active');
            Object.assign(item.style, {
                backgroundColor: 'transparent',
                boxShadow: 'none'
            });
        });

        // Add active class to selected item
        const activeItem = items[index];
        if (activeItem) {
            activeItem.classList.add('active');
            Object.assign(activeItem.style, {
                backgroundColor: 'rgba(61, 110, 255, 0.2)',
                boxShadow: 'inset 0 0 10px rgba(12, 255, 255, 0.3)'
            });

            // Scroll item into view if necessary
            activeItem.scrollIntoView({
                block: 'nearest'
            });
        }
    }

    /**
     * Load recent searches from settings
     * @returns {Array} Array of recent searches
     */
    loadRecentSearches() {
        try {
            const settings = CUIBlockchainExplorer.getSettings();
            if (settings && settings.getData && settings.getData.recentSearches) {
                return settings.getData.recentSearches;
            }
        } catch (error) {
            console.warn('Error loading recent searches:', error);
        }
        return [];
    }

    /**
     * Add search term to recent searches
     * @param {string} search Search term to add
     */
    addRecentSearch(search) {
        if (!search || search.trim() === '')
            return;

        try {
            // Remove if already exists
            this.recentSearches = this.recentSearches.filter(s => s !== search);

            // Add to beginning of array
            this.recentSearches.unshift(search);

            // Limit to 10 items
            this.recentSearches = this.recentSearches.slice(0, 10);

            // Save to settings
            const settings = CUIBlockchainExplorer.getSettings();
            if (settings && settings.getData) {
                settings.getData.recentSearches = this.recentSearches;
                this.mInstance.saveSettings();
            }
        } catch (error) {
            console.warn('Error saving recent searches:', error);
        }
    }

    /**
     * Escape special characters in string for use in RegExp
     * @param {string} string String to escape
     * @returns {string} Escaped string
     */
    escapeRegExp(string) {
        return string.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
    }

    /**
     * Clean up autocomplete resources
     */
    cleanupAutoComplete() {
        // Remove event listeners for search input
        if (this.elements.searchInput && this.searchAutoCompleteHandlers) {
            const input = this.elements.searchInput;
            const handlers = this.searchAutoCompleteHandlers;

            input.removeEventListener('focus', handlers.focus);
            input.removeEventListener('input', handlers.input);
            input.removeEventListener('blur', handlers.blur);
            input.removeEventListener('keydown', handlers.keydown);
        }

        // Remove event listeners for domain search input
        if (this.elements.domainSearchInput && this.domainAutoCompleteHandlers) {
            const input = this.elements.domainSearchInput;
            const handlers = this.domainAutoCompleteHandlers;

            input.removeEventListener('focus', handlers.focus);
            input.removeEventListener('input', handlers.input);
            input.removeEventListener('blur', handlers.blur);
            input.removeEventListener('keydown', handlers.keydown);
        }

        // Remove containers
        if (this.searchAutoCompleteContainer) {
            this.searchAutoCompleteContainer.parentNode.removeChild(this.searchAutoCompleteContainer);
        }

        if (this.domainAutoCompleteContainer) {
            this.domainAutoCompleteContainer.parentNode.removeChild(this.domainAutoCompleteContainer);
        }

        // Clear references
        this.searchAutoCompleteContainer = null;
        this.domainAutoCompleteContainer = null;
        this.searchAutoCompleteHandlers = null;
        this.domainAutoCompleteHandlers = null;
    }
    // AutoComplete - END

    /**
     * Initialize the UI dApp
     */
    async initialize() {
        if (this.initialized)
            return;

        try {
            // request Main VM thread
            //this.mVMContext.processGridScript(); // causes main VM Thread to be spawned

            // Threading - BEGIN
            let systemThreadID = this.mVMContext.getSystemThreadID; // main system thread
            let dataThreadID = this.mVMContext.getDataThreadID; // main system thread
            //let appPrivateThreadID = await this.mVMContext.createThreadA(0, this); // if app desired to create a private thread


            if (!this.mTools.isThreadIDValid(systemThreadID)) {
                systemThreadID = await this.mVMContext.wakeThreadA(0, this); // just make sure main thread is available.
            }
            // Threading - END

            // Load settings first
            if (this.loadSettings()) {
                this.mTools.logEvent('[' + CUIBlockchainExplorer.getPackageID() + ']\'s settings activated!',
                    eLogEntryCategory.dApp, 0, eLogEntryType.notification);
            } else {
                CUIBlockchainExplorer.setSettings(CUIBlockchainExplorer.getDefaultSettings());
                this.mTools.logEvent('Failed to activate provided settings for [' + this.getPackageID() + ']. Assuming defaults!',
                    eLogEntryCategory.dApp, 0, eLogEntryType.failure);
            }

            // Initialize UI elements
            this.initUIElements();

            // Import and initialize the search configuration panel
            this.initSearchConfigPanel().then(() => {
                // Initialize UI components with proper error handling
                this.ui = new BlockchainExplorerUI(this, this.elements, this.enums, this.paginationState);
                if (this.ui) {
                    this.ui.registerCallbacks({
                        onGetMarketDomainData: this.fetchMarketData.bind(this),
                        onSearch: this.performSearch.bind(this),
                        onDomainSearch: this.searchDomain.bind(this),
                        onViewBlockDetails: this.viewBlockDetails.bind(this),
                        onViewTransactionDetails: this.viewTransactionDetails.bind(this),
                        onSectionChange: this.onSectionChange.bind(this),
                        onPageChange: this.handlePageChange.bind(this),
                        onPageSizeChange: this.handlePageSizeChange.bind(this),
                        onError: this.handleError.bind(this)
                    });

                    try {
                        this.ui.initialize();
                    } catch (error) {
                        console.error("Error initializing UI:", error);
                    }
                }
                this.ui.showLoading(true);
                setTimeout(() => {
                    this.ui.showLoading(false);
                }, 5000); // fallback


                // Show loading while initializing
                this.ui.showLoading(true);

                this.dataManager.initEventListeners(this.getWinID);

                // Start controller thread
                this.mControler = CVMContext.getInstance().createJSThread(this.controllerThreadF.bind(this), this.getProcessID, this.mControllerThreadInterval);

                // Initialize custom autocomplete
                this.initAutoComplete();

                // Mark as initialized
                this.initialized = true;

                // Process GLink if launched via deep link
                // Use setTimeout instead of waiting for fetchInitialData since it may hang
                if (this.wasLaunchedViaGLink) {
                    console.log('>>> [Explorer GLink] Detected launch via GLink');
                    const glinkData = this.getGLinkData;
                    console.log('>>> [Explorer GLink] Data retrieved:', JSON.stringify(glinkData));

                    // Accept the GLink early to extend timeout
                    const glinkHandler = CGLinkHandler.getInstance();
                    glinkHandler.acceptGLink('Explorer initializing, loading data...');
                    console.log('>>> [Explorer GLink] Accepted GLink, timeout extended');

                    // Process GLink after a short delay to allow UI to stabilize
                    // Don't depend on fetchInitialData completing as it may hang
                    const self = this;
                    setTimeout(() => {
                        console.log('>>> [Explorer GLink] Processing action after 2s delay...');
                        self.processGLink(glinkData);
                    }, 2000); // 2 second delay for UI to initialize
                }

                // Fetch initial data after everything is initialized
                this.fetchInitialData().catch(error => {
                    console.error("Error fetching initial data:", error);
                    this.handleError("Failed to load blockchain data. Please try again later.");
                });
            }).catch(error => {
                console.error("Error initializing search config:", error);
                this.ui.showLoading(false);
            });

        } catch (error) {
            console.error("Critical initialization error:", error);
            this.handleError("A critical error occurred during initialization. Please try again later.");
            this.ui.showLoading(false);
        }
    }

    /**
     * Initialize search configuration panel
     * @returns {Promise} Promise that resolves when the panel is initialized
     */
    async initSearchConfigPanel() {
        return new Promise((resolve, reject) => {
            try {
                import('./SearchConfigPanel.js').then(module => {
                    const SearchConfigPanel = module.default;

                    // Create container for search config panel
                    const searchConfigContainer = document.createElement('div');
                    searchConfigContainer.id = 'search-config-container';
                    searchConfigContainer.style.display = 'none'; // Hidden by default

                    // Add it after the search section
                    const searchSection = this.getControl('search-section') || $(this.getBody).find('.search-section')[0];
                    if (searchSection) {
                        searchSection.parentNode.insertBefore(searchConfigContainer, searchSection.nextSibling);

                        // Initialize the search config panel
                        this.searchConfigPanel = new SearchConfigPanel(
                                searchConfigContainer,
                                this.enums,
                                this.onSearchConfigChange.bind(this));

                        // Add toggle button to search section
                        const toggleButton = document.createElement('button');
                        toggleButton.className = 'search-button';
                        toggleButton.textContent = 'Advanced';
                        toggleButton.style.marginLeft = '0.5rem';

                        toggleButton.addEventListener('click', function () {
                            const isVisible = searchConfigContainer.style.display !== 'none';
                            searchConfigContainer.style.display = isVisible ? 'none' : 'block';
                            toggleButton.textContent = isVisible ? 'Advanced' : 'Simple';

                            // Reset the search config panel when hiding it
                            if (isVisible) {
                                this.searchConfigPanel.reset();
                            }
                        }
                            .bind(this)); // Bind this regular function to the class instance

                        // Add button to search section
                        const searchButton = this.getControl('search-button');
                        if (searchButton) {
                            searchButton.parentNode.insertBefore(toggleButton, searchButton.nextSibling);
                        }

                        resolve();
                    } else {
                        console.warn("Search section not found");
                        resolve(); // Resolve anyway to continue initialization
                    }
                }).catch(error => {
                    console.error("Error loading search configuration panel:", error);
                    resolve(); // Resolve anyway to continue initialization
                });
            } catch (error) {
                console.error("Error initializing search panel:", error);
                resolve(); // Resolve anyway to continue initialization
            }
        });
    }

    /**
     * Handle data updates from data manager (general purpose handler for any type of a request / response data pattern).
     * @param {number} type - Update type
     * @param {*} data - Updated data
     * @param {Object} [metadata] - Metadata about the request/response
     */
    async handleDataUpdate(type, data, metadata) {
        try {

            // Update pagination metadata if provided
            if (metadata && metadata.pagination) {
                this.updatePaginationState(type, metadata.pagination);
            }

            // For transaction-related updates, update statuses first
            if (this.isTransactionData(type, data)) {
                await this.updateAllTransactionStatuses(data); // this is async call for transaction data only.
            }

            // Convert data to plain objects after updating statuses
            const plainData = Array.isArray(data) ?
                this.mTools.convertBlockchainMetaToPlainObject(data) : data;

            // Update UI based on the update type
            switch (type) {
            case this.enums.eRequestType.BLOCKCHAIN_STATUS:
            case this.enums.eRequestType.LIVENESS:
            case this.enums.eRequestType.HEIGHT:
            case this.enums.eRequestType.KEY_HEIGHT:
            case this.enums.eRequestType.MARKET_CAP:
            case this.enums.eRequestType.USDT_PRICE:
                // Also update transaction statuses when height changes
                if (type === this.enums.eRequestType.KEY_HEIGHT) {
                    this.ui.refreshTableTransactionStatuses(plainData); // Rationale: once relevant context data is retrieved, we update statuses of transactions as well
                }
                this.ui.updateStatusDisplay({
                    blockchainStatus: type === this.enums.eRequestType.BLOCKCHAIN_STATUS ? plainData : undefined,
                    height: type === this.enums.eRequestType.HEIGHT ? plainData : undefined,
                    keyHeight: type === this.enums.eRequestType.KEY_HEIGHT ? plainData : undefined,
                    usdtPrice: type === this.enums.eRequestType.USDT_PRICE ? plainData : undefined,
                    liveness: type === this.enums.eRequestType.LIVENESS ? plainData : undefined,
                    marketCap: type === this.enums.eRequestType.MARKET_CAP ? plainData : undefined,
                });
                break;

            case this.enums.eRequestType.NETWORK_UTILIZATION:
            case this.enums.eRequestType.BLOCK_SIZE:
            case this.enums.eRequestType.BLOCK_REWARDS:
            case this.enums.eRequestType.AVERAGE_BLOCK_TIME:
                this.ui.updateDashboardDisplay({
                    networkUtilization: type === this.enums.eRequestType.NETWORK_UTILIZATION ? plainData : undefined,
                    blockSize: type === this.enums.eRequestType.BLOCK_SIZE ? plainData : undefined,
                    blockRewards: type === this.enums.eRequestType.BLOCK_REWARDS ? plainData : undefined,
                    avgBlockTime: type === this.enums.eRequestType.AVERAGE_BLOCK_TIME ? plainData : undefined
                });
                this.ui.updateStatisticsDisplay({
                    networkUtilization: type === this.enums.eRequestType.NETWORK_UTILIZATION ? plainData : undefined,
                    blockSize: type === this.enums.eRequestType.BLOCK_SIZE ? plainData : undefined,
                    blockRewards: type === this.enums.eRequestType.BLOCK_REWARDS ? plainData : undefined
                });
                break;

            case this.enums.eRequestType.AVERAGE_KEY_BLOCK_TIME:
                this.ui.updateStatisticsDisplay({
                    avgKeyBlockTime: plainData
                });
                break;

            case this.enums.eRequestType.RECENT_BLOCKS:
                // Ensure plainData is an array before passing to Tabulator
                if (Array.isArray(plainData)) {
                    if (this.ui.tables.recentBlocks) {
                        this.pauseCurtain();
                        this.ui.tables.recentBlocks.setData(plainData);
                    }
                    if (this.ui.tables.blocks) {
                        this.pauseCurtain();
                        this.ui.tables.blocks.setData(plainData);

                        // Update pagination
                        if (metadata && metadata.pagination) {
                            this.ui.updatePagination('blocks', metadata.pagination);
                        }
                    }
                } else {
                    console.warn('>>> [Explorer] RECENT_BLOCKS received non-array data:', typeof plainData);
                }
                break;

            case this.enums.eRequestType.RECENT_TRANSACTIONS:
                // Ensure plainData is an array before passing to Tabulator
                if (Array.isArray(plainData)) {
                    if (this.ui.tables.recentTransactions) {
                        this.pauseCurtain();
                        this.ui.tables.recentTransactions.setData(plainData);
                    }

                    this.ui.updateDailyStatsChart(plainData);

                    if (this.ui.tables.transactions) {
                        this.pauseCurtain();
                        this.ui.tables.transactions.setData(plainData);

                        // Update pagination
                        if (metadata && metadata.pagination) {
                            this.ui.updatePagination('transactions', metadata.pagination);
                        }
                    }
                } else {
                    console.warn('>>> [Explorer] RECENT_TRANSACTIONS received non-array data:', typeof plainData);
                }
                break;

            case this.enums.eRequestType.TRANSACTION_DAILY_STATS:
                this.ui.updateTransactionsChart(plainData);

                break;

            case this.enums.eRequestType.BLOCK_DETAILS:
                // Convert single object for block details if needed
                const blockDetailsData = (plainData instanceof CBlockDesc || (plainData.constructor && plainData.constructor.name === 'CBlockDesc')) ?
                this.mTools.convertBlockchainMetaToPlainObject([plainData])[0] : plainData;
                this.ui.displayBlockDetails(blockDetailsData);
                break;

            case this.enums.eRequestType.TRANSACTION_DETAILS:
                // Convert single object for transaction details if needed
                const txDetailsData = (plainData instanceof CTransactionDesc || (plainData.constructor && plainData.constructor.name === 'CTransactionDesc')) ?
                this.mTools.convertBlockchainMetaToPlainObject([plainData])[0] : plainData;
                this.ui.displayTransactionDetails(txDetailsData);
                break;

            case this.enums.eRequestType.DOMAIN_DETAILS:
                // Convert single object for domain details if needed
                const domainDetailsData = (plainData instanceof CDomainDesc || (plainData.constructor && plainData.constructor.name === 'CDomainDesc')) ?
                this.mTools.convertBlockchainMetaToPlainObject([plainData])[0] : plainData;
                this.ui.displayDomainDetails(domainDetailsData);
                break;

            case this.enums.eRequestType.MARKET_DATA:

                if (this.ui.tables.domainHistory) {
                    this.ui.tables.domainHistory.setData(plainData);

                    // Update pagination if provided
                    if (metadata && metadata.pagination) {
                        this.ui.updatePagination('domainHistory', metadata.pagination);
                    }

                    // Update UI to show we're viewing market data
                    this.ui.showMarketDataView();
                }

                this.ui.updateStatusDisplay({
                    marketCap: type === this.enums.eRequestType.MARKET_CAP ? plainData : undefined,
                });

                break;

            case this.enums.eRequestType.DOMAIN_HISTORY:
                if (this.ui.tables.domainHistory) {
                    this.pauseCurtain();
                    this.ui.tables.domainHistory.setData(plainData);

                    // Update pagination
                    if (metadata && metadata.pagination) {
                        this.ui.updatePagination('domainHistory', metadata.pagination);
                    }
                }
                break;

            case this.enums.eRequestType.SEARCH:
                this.ui.displaySearchResults(plainData, this.lastSearchQuery);

                // Update pagination
                if (metadata && metadata.pagination) {
                    this.ui.updatePagination('search', metadata.pagination);
                }
                break;

            default:
                console.log(`Unhandled update type: ${type}`);
                break;
            }
        } catch (error) {

            console.error(`Error handling data update (type ${type}):`, error);
            this.handleError(error);
        }
    }

    /**
     * Checks if the data contains transaction information
     * @param {number} type - The update type
     * @param {*} data - The data to check
     * @returns {boolean} - True if transaction-related
     */
    isTransactionData(type, data) {
        // Check if data type is transaction-related
        const txTypes = [
            this.enums.eRequestType.RECENT_TRANSACTIONS,
            this.enums.eRequestType.TRANSACTION_DETAILS,
            this.enums.eRequestType.DOMAIN_HISTORY
        ];

        if (txTypes.includes(type)) {
            return true;
        }

        // Also check if data contains transactions
        if (Array.isArray(data)) {
            return data.some(item =>
                item instanceof CTransactionDesc ||
                (item.constructor && item.constructor.name === 'CTransactionDesc'));
        }

        // Check if single item is a transaction
        return data instanceof CTransactionDesc ||
        (data && data.constructor && data.constructor.name === 'CTransactionDesc');
    }

    /**
     * Updates statuses of all transactions in data
     * @param {*} data - Data containing transactions
     * @returns {Promise<void>}
     */
    async updateAllTransactionStatuses(data) {
        // Handle arrays of data
        if (Array.isArray(data)) {
            const transactions = data.filter(item =>
                    item instanceof CTransactionDesc ||
                    (item.constructor && item.constructor.name === 'CTransactionDesc'));

            if (transactions.length > 0) {
                // Update all transaction statuses in parallel
                const updatePromises = transactions.map(tx =>
                        tx.updateStatus().catch(error => {
                            console.warn(`Error updating transaction status:`, error);
                        }));

                await Promise.all(updatePromises);
            }

            // Handle nested transactions (e.g., in block details)
            for (const item of data) {
                if (item && item.transactions && Array.isArray(item.transactions)) {
                    await this.updateAllTransactionStatuses(item.transactions);
                }
            }
        }
        // Handle single transaction object
        else if (data instanceof CTransactionDesc ||
            (data && data.constructor && data.constructor.name === 'CTransactionDesc')) {
            await data.updateStatus().catch(error => {
                console.warn(`Error updating transaction status:`, error);
            });
        }
        // Handle objects that might contain transaction arrays
        else if (data && typeof data === 'object') {
            for (const key in data) {
                if (Array.isArray(data[key]) &&
                    data[key].some(item => item instanceof CTransactionDesc)) {
                    await this.updateAllTransactionStatuses(data[key]);
                }
            }
        }
    }

    /**
     * Update pagination state based on response metadata
     * @param {number} requestType - Request type
     * @param {Object} pagination - Pagination metadata
     */
    updatePaginationState(requestType, pagination) {
        let stateKey = null;

        switch (requestType) {
        case this.enums.eRequestType.RECENT_BLOCKS:
            stateKey = 'blocks';
            break;

        case this.enums.eRequestType.RECENT_TRANSACTIONS:
            stateKey = 'transactions';
            break;

        case this.enums.eRequestType.DOMAIN_HISTORY:
            stateKey = 'domainHistory';
            break;

        case this.enums.eRequestType.SEARCH:
            stateKey = 'search';
            break;

        default:
            return; // Not a paginated request type
        }

        if (stateKey && this.paginationState[stateKey]) {
            // Update state with new pagination info
            Object.assign(this.paginationState[stateKey], {
                currentPage: pagination.currentPage || this.paginationState[stateKey].currentPage,
                pageSize: pagination.pageSize || this.paginationState[stateKey].pageSize,
                totalPages: pagination.totalPages || this.paginationState[stateKey].totalPages,
                totalItems: pagination.totalItems || this.paginationState[stateKey].totalItems
            });
        }
    }

    /**
     * Handle pagination page change
     * @param {string} section - Section identifier (blocks, transactions, etc.)
     * @param {number} newPage - New page number
     */
    handlePageChange(type, page) {
        if (!this.paginationState[type])
            return;

        this.paginationState[type].currentPage = page;

        // Special handling for market data view
        if (type === 'domainHistory' && this.paginationState.domainHistory.isMarketDataView) {
            this.fetchMarketData(page);
            return;
        }

        // Normal page fetch for other views
        this.fetchPagedData(type, page);
    }

    /**
     * Handle pagination page size change
     * @param {string} section - Section identifier (blocks, transactions, etc.)
     * @param {number} newSize - New page size
     */
    handlePageSizeChange(section, newSize) {
        if (!this.paginationState[section])
            return;

        // Update pagination state
        this.paginationState[section].pageSize = newSize;
        this.paginationState[section].currentPage = 1; // Reset to first page

        // Save to settings if needed
        let sets = CUIBlockchainExplorer.getSettings();
        if (sets && sets.getData) {
            sets.getData.defaultPageSize = newSize;
            this.saveSettings();
        }

        // Fetch data with new page size
        this.fetchPagedData(section, 1);
    }

    /**
     * Fetch data for a specific page
     * @param {string} section - Section identifier (blocks, transactions, etc.)
     * @param {number} page - Page number to fetch
     */
    fetchPagedData(section, page) {
        this.ui.showLoading(true);

        try {
            switch (section) {
            case 'blocks':
                this.dataManager.requestRecentBlocks(
                    this.paginationState.blocks.pageSize,
                    page,
                    this.paginationState.blocks.sortBy).catch(error => {
                    console.error("Error fetching blocks page:", error);
                    this.handleError(error);
                }).finally(() => {
                    this.ui.showLoading(false);
                });
                break;

            case 'transactions':
                this.dataManager.requestRecentTransactions(
                    this.paginationState.transactions.pageSize,
                    page).catch(error => {
                    console.error("Error fetching transactions page:", error);
                    this.handleError(error);
                }).finally(() => {
                    this.ui.showLoading(false);
                });
                break;

            case 'domainHistory':
                if (this.currentDomain) {
                    this.dataManager.requestDomainHistory(
                        this.currentDomain,
                        this.paginationState.domainHistory.pageSize,
                        page,
                        this.paginationState.domainHistory.sortBy).catch(error => {
                        console.error("Error fetching domain history page:", error);
                        this.handleError(error);
                    }).finally(() => {
                        this.ui.showLoading(false);
                    });
                } else {
                    this.ui.showLoading(false);
                }
                break;

            case 'search':
                if (this.paginationState.search.lastQuery) {
                    this.dataManager.searchBlockchain(
                        this.paginationState.search.lastQuery,
                        this.paginationState.search.lastFilter,
                        this.paginationState.search.pageSize,
                        page).catch(error => {
                        console.error("Error fetching search results page:", error);
                        this.handleError(error);
                    }).finally(() => {
                        this.ui.showLoading(false);
                    });
                } else {
                    this.ui.showLoading(false);
                }
                break;

            default:
                this.ui.showLoading(false);
                break;
            }
        } catch (error) {
            console.error(`Error handling page change for ${section}:`, error);
            this.handleError(error);
            this.ui.showLoading(false);
        }
    }

    /**
     * Handle errors
     * @param {Error|string} error - Error to handle
     */
    handleError(error) {
        //try {
        // Format error message
        const message = typeof error === 'string' ? error : (error.message || 'Unknown error occurred');
        this.mErrorMsg = message;

        // Log the error for debugging
        console.error("Blockchain Explorer Error:", message, error);
        /*
        // Show error in UI if available
        if (this.initialized && this.ui && typeof this.ui.showError === 'function') {
        this.ui.showError(message);
        } else {
        // Fallback to built-in dialog if UI isn't ready
        this.askString('Error', message, () => {}, false);
        }

        // Hide loading if visible
        if (this.initialized && this.ui && typeof this.ui.showLoading === 'function') {
        this.ui.showLoading(false);
        }
        } catch (e) {
        // Last resort error handling
        console.error("Error in error handler:", e);
        try {
        this.askString('Error', 'An unexpected error occurred', () => {}, false);
        } catch (dialogError) {
        console.error("Could not display error dialog:", dialogError);
        }
        }*/
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
                    this.dataManager.requestMarketDepth(),
                    this.dataManager.requestLiveness(),
                    this.dataManager.requestHeight(),
                    this.dataManager.requestUSDTPrice(),
                    this.dataManager.requestRecentBlocks(
                        this.paginationState.blocks.pageSize,
                        this.paginationState.blocks.currentPage,
                        this.paginationState.blocks.sortBy),
                    this.dataManager.requestRecentTransactions(
                        this.paginationState.transactions.pageSize,
                        this.paginationState.transactions.currentPage),
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
        try {
            if (this.mControlerExecuting || !this.initialized) {
                return false;
            }

            this.mControlerExecuting = true; // mutex protection

            try {
                // Safety check to ensure UI is initialized
                if (!this.ui || !this.dataManager) {
                    console.warn("UI or data manager not initialized, skipping controller update");
                    return false;
                }

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
                    let txPageSize = 50;
                    let blockPageSize = 50;

                    // Load User Pagination Settings - BEGIN
                    let sets = CUIBlockchainExplorer.getSettings();
                    if (sets && sets.getData) {
                        txPageSize = sets.getData.defaultPageSize;
                        blockPageSize = sets.getData.defaultPageSize;
                    }
                    // Load User Pagination Settings - BEGIN

                    // Update dashboard data
                    Promise.all([
                            this.dataManager.requestRecentBlocks(blockPageSize),
                            this.dataManager.requestRecentTransactions(txPageSize),
                            this.dataManager.requestNetworkUtilization(),
                            this.dataManager.requestBlockSize(),
                            this.dataManager.requestBlockRewards(),
                            this.dataManager.requestAverageBlockTime()
                        ]).catch(error => {
                        console.error("Error updating dashboard:", error);
                    });
                }
            } catch (error) {
                console.error("Error in controller thread:", error);
            } finally {
                this.mControlerExecuting = false;
            }

            return true;
        } catch (error) {
            console.error("Critical error in controller thread:", error);
            this.mControlerExecuting = false;
            return false;
        }
    }

    /**
     * Get the currently active section
     * @returns {string} Active section ID
     */
    getActiveSection() {
        try {
            // Use mInstance to properly access Shadow DOM elements
            const activeNavItem =
                $(this.getBody).find('.nav-item.active')[0];

            return activeNavItem ? activeNavItem.getAttribute('data-section') : 'dashboard';
        } catch (error) {
            console.warn("Error determining active section:", error);
            return 'dashboard'; // Default to dashboard
        }
    }

    /**
     * Fetch and display market data
     * @param {number} page - Page number
     */
    async fetchMarketData(page = 1) {
        this.ui.showLoading(true);

        try {
            // Get sort settings from UI
            const sortType = parseInt(this.elements.marketSortType.value || "1", 10);

            const sortOrder = parseInt(this.elements.marketSortOrder.value || "1", 10);

            // Clear current domain details
            this.currentDomain = "";
            const domainDetails = this.getControl('domain-details');
            if (domainDetails) {
                domainDetails.innerHTML = '<h3>Top Domains</h3>';
            }

            // Set the pagination state
            this.paginationState.domainHistory.currentPage = page;
            this.paginationState.domainHistory.isMarketDataView = true; // Flag for pagination handler

            // Request market data
            await this.dataManager.requestMarketData(
                page,
                this.paginationState.domainHistory.pageSize,
                sortType,
                sortOrder);

            // Show domains section
            this.ui.switchToSection('domains');

            // Ensure we're in market data view
            this.ui.showMarketDataView();
        } catch (error) {
            console.error("Error fetching market data:", error);
            this.handleError(`Failed to fetch market data: ${error.message}`);
        } finally {
            this.ui.showLoading(false);
        }
    }

    /**
     * Perform search based on user input
     * @param {string} query - Search query
     */
    async performSearch(query) {

        // Get CTools instance for validation
        const tools = CTools.getInstance();

        // Create a proper search filter based on current configuration
        let searchFilter = null;

        if (this.searchConfigPanel) {
            // Create a search filter from the current configuration
            searchFilter = this.searchConfigPanel.createSearchFilter(CSearchFilter);
            // Store for pagination
            this.paginationState.search.lastFilter = searchFilter;
        }

        if (searchFilter.getArbitraryFlagCount() == 0 && (!query || query.trim() === ''))
            return;

        this.ui.showLoading(true);
        const cleanQuery = query.trim();
        this.lastSearchQuery = cleanQuery;

        // Store for pagination
        this.paginationState.search.lastQuery = cleanQuery;
        this.paginationState.search.currentPage = 1;

        try {

            // Determine search strategy based on query format

            // Check if it's a domain (containing a dot and passing validation) - base-58 decodeding + checksum validation
            if (searchFilter.hasStandardFlag(CSearchFilter.StandardFlags.DOMAINS) && tools.isDomainIDValid && tools.isDomainIDValid(cleanQuery)) {
                try {
                    await this.searchDomain(cleanQuery);
                    return;
                } catch (domainError) {
                    console.log("Domain search failed, falling back to general search:", domainError);
                    // Fall through to general search
                }
            }
            // Check if it could be a receipt ID
            else if (searchFilter.hasStandardFlag(CSearchFilter.StandardFlags.TRANSACTIONS) && tools.isReceiptIDValid) {
                const receiptCheck = tools.isReceiptIDValid(cleanQuery); //- base-58 decodeding + checksum validation
                if (receiptCheck && receiptCheck.isValid) {
                    try {
                        // Try to use the decoded receipt ID as a transaction ID
                        // const decodedStr = new TextDecoder().decode(receiptCheck.decoded);
                        await this.viewTransactionDetails(cleanQuery);
                        return;
                    } catch (receiptError) {
                        console.log("Receipt ID didn't match a transaction, falling back to general search");
                        // Fall through to general search
                    }
                }
            }

            // Default: perform a general blockchain search
            await this.dataManager.searchBlockchain(
                cleanQuery,
                searchFilter,
                this.paginationState.search.pageSize,
                this.paginationState.search.currentPage);

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
            const blockDetails = await this.mVMContext.getBlockDetailsA(blockId, this.getThreadID, this, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

            this.ui.switchToSection('blocks', true);

            let plainData = this.mTools.convertBlockchainMetaToPlainObject(blockDetails); ;
            this.ui.displayBlockDetails(plainData);

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
        console.log('>>> [Explorer] viewTransactionDetails called with txId:', transactionId);
        this.ui.showLoading(true);

        try {
            console.log('>>> [Explorer] Fetching transaction details from network...');
            const txDetails = await this.mVMContext.getTransactionDetailsA(transactionId, this.getThreadID, this, this.enums.eVMMetaCodeExecutionMode.RAW, 0);
            console.log('>>> [Explorer] Transaction details received:', txDetails ? 'OK' : 'null');
            console.log('>>> [Explorer] txDetails type:', txDetails?.constructor?.name);

            if (!txDetails) {
                console.error('>>> [Explorer] No transaction details returned');
                this.handleError(`Transaction not found: ${transactionId}`);
                return null;
            }

            if (txDetails instanceof CTransactionDesc) {
                console.log('>>> [Explorer] Is CTransactionDesc, updating status...');
                await txDetails.updateStatus(); // wait for local autonomous safety assessment to complete
                console.log('>>> [Explorer] After updateStatus - verifiableID:', txDetails.verifiableID, 'sender:', txDetails.sender, 'status:', txDetails.status);
            } else {
                console.log('>>> [Explorer] NOT a CTransactionDesc instance, raw props:', Object.keys(txDetails));
            }

            // Switch to transactions section first
            console.log('>>> [Explorer] Switching to transactions section...');
            this.ui.switchToSection('transactions', true);

            // Convert to plain object - wrap in array and take first element for consistent handling
            // This matches how handleDataUpdate processes TRANSACTION_DETAILS
            let plainData;
            if (txDetails instanceof CTransactionDesc || (txDetails.constructor && txDetails.constructor.name === 'CTransactionDesc')) {
                plainData = this.mTools.convertBlockchainMetaToPlainObject([txDetails])[0];
            } else {
                // Fallback: try direct conversion for single object
                plainData = this.mTools.convertBlockchainMetaToPlainObject(txDetails);
            }

            console.log('>>> [Explorer] Converted plainData keys:', plainData ? Object.keys(plainData) : 'null');
            console.log('>>> [Explorer] plainData.verifiableID:', plainData?.verifiableID);
            console.log('>>> [Explorer] plainData.sender:', plainData?.sender);

            // Ensure DOM is ready before displaying (small delay for view transition)
            await new Promise(resolve => requestAnimationFrame(resolve));

            console.log('>>> [Explorer] Displaying transaction details...');
            this.ui.displayTransactionDetails(plainData);

            console.log('>>> [Explorer] viewTransactionDetails completed successfully');
            return txDetails;
        } catch (error) {
            console.error('>>> [Explorer] Error viewing transaction details:', error);
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
        if (!domain || domain.trim() === '')
            return;

        this.ui.showLoading(true);

        try {
            // Store current domain for pagination
            this.currentDomain = domain;

            // Reset domain pagination
            this.paginationState.domainHistory.currentPage = 1;

            // Get domain details
            const domainDetails = await this.dataManager.requestDomainDetails(domain);

            // Get domain transaction history with pagination
            const transactions = await this.dataManager.requestDomainHistory(
                    domain,
                    this.paginationState.domainHistory.pageSize,
                    this.paginationState.domainHistory.currentPage,
                    this.paginationState.domainHistory.sortBy);

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
        if (this.subscriptionActive)
            return;

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
        if (!this.subscriptionActive)
            return;

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
        // Skip if not initialized
        if (!this.initialized) {
            console.warn("UI not fully initialized, section change postponed");
            return;
        }

        try {
            // Show loading while data loads
            this.ui.showLoading(true);

            // Load section-specific data with proper error handling
            switch (sectionId) {
            case 'dashboard':
                // Dashboard data is loaded on init and by controller thread
                this.ui.showLoading(false);
                break;

            case 'blocks':
                this.dataManager.requestRecentBlocks(
                    this.paginationState.blocks.pageSize,
                    this.paginationState.blocks.currentPage,
                    this.paginationState.blocks.sortBy).catch(error => {
                    console.error("Error loading blocks data:", error);
                    this.handleError("Failed to load blocks. Please try again later.");
                }).finally(() => {
                    this.ui.showLoading(false);
                });
                break;

            case 'transactions':
                this.dataManager.requestRecentTransactions(
                    this.paginationState.transactions.pageSize,
                    this.paginationState.transactions.currentPage).catch(error => {
                    console.error("Error loading transactions data:", error);
                    this.handleError("Failed to load transactions. Please try again later.");
                }).finally(() => {
                    this.ui.showLoading(false);
                });
                break;

            case 'domains':
                if (this.currentDomain) {
                    // If we have a specific domain, load its history
                    this.dataManager.requestDomainHistory(
                        this.currentDomain,
                        this.paginationState.domainHistory.pageSize,
                        this.paginationState.domainHistory.currentPage,
                        this.paginationState.domainHistory.sortBy).catch(error => {
                        console.error("Error refreshing domain history:", error);
                        this.handleError("Failed to load domain history. Please try again later.");
                    }).finally(() => {
                        this.ui.showLoading(false);
                    });
                } else {
                    // If no specific domain, show market data
                    this.fetchMarketData(this.paginationState.domainHistory.currentPage);
                }
                break;

            case 'statistics':
                Promise.all([
                        this.dataManager.requestTransactionDailyStats(30),
                        this.dataManager.requestNetworkUtilization(),
                        this.dataManager.requestBlockSize(),
                        this.dataManager.requestBlockRewards(),
                        this.dataManager.requestAverageBlockTime(),
                        this.dataManager.requestAverageKeyBlockTime()
                    ]).catch(error => {
                    console.error("Error loading statistics data:", error);
                    this.handleError("Failed to load statistics. Please try again later.");
                }).finally(() => {
                    this.ui.showLoading(false);
                });
                break;

            default:
                this.ui.showLoading(false);
                break;
            }

            // Give UI time to update after section change
            setTimeout(() => {
                if (this.ui && this.ui.refreshTables) {
                    try {
                        this.ui.refreshTables(sectionId);
                    } catch (error) {
                        console.error("Error refreshing tables after section change:", error);
                    }
                }
            }, 300);

        } catch (error) {
            console.error("Error handling section change:", error);
            this.ui.showLoading(false);
        }
    }

    /**
     * Handle window resize event
     * @param {boolean} isFallbackEvent - Whether this is a fallback event
     */
    finishResize(isFallbackEvent) {

        // Ensure we don't proceed if UI isn't initialized
        if (!this.initialized || !this.ui) {
            console.log("UI not initialized yet, skipping resize operations");
            return;
        }

        try {
            // Get current client dimensions
            const height = this.getClientHeight;
            const width = this.getClientWidth;

            if (this.ui && typeof this.ui.handleResize === 'function') {
                this.ui.handleResize(width, height);
            }

            if (!isFallbackEvent) {
                if (width < 950) {
                    this.animateCSS("header-title", "fadeIn", 'animate__', null, false, true);
                } else {
                    this.animateCSS("header-title", "fadeOut", 'animate__', null, false, false);
                }
            }
            /*
            // Safety check to make sure dimensions are valid
            if (height <= 0 || width <= 0) {
            console.warn("Invalid dimensions, skipping resize:", width, height);
            return;
            }

            // Update UI based on new dimensions using requestAnimationFrame to ensure DOM is ready
            window.requestAnimationFrame(() => {
            if (this.ui && typeof this.ui.handleResize === 'function') {
            try {
            this.ui.handleResize(width, height);
            } catch (error) {
            console.error("Error during UI resize:", error);
            }
            }
            });*/
        } catch (error) {
            console.error("Error in finishResize:", error);
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

        // Clean up autocomplete
        this.cleanupAutoComplete();
        super.closeWindow();
    }

    /**
     * User provided response to a data query
     * @param {Object} e - Response event
     */
    userResponseCallback(e) {
        console.log('User answered:', e.answer);
    }

    // === GLink Support - BEGIN ===

    /**
     * Process GLink data - handle deep link navigation
     * @override
     * @param {Object} glinkData - The parsed GLink data {view, action, data}
     * @returns {boolean} True if successfully processed
     */
    processGLink(glinkData) {
        const glinkHandler = CGLinkHandler.getInstance();

        if (!glinkData) {
            console.warn('>>> [Explorer GLink] No GLink data to process');
            glinkHandler.rejectGLink('No GLink data provided');
            return false;
        }

        console.log('>>> [Explorer GLink] processGLink called with:', JSON.stringify(glinkData));

        // Accept the GLink - acknowledge receipt (in case not already accepted)
        glinkHandler.acceptGLink('Explorer processing...');

        const { view, action, data } = glinkData;
        console.log('>>> [Explorer GLink] Parsed - view:', view, 'action:', action, 'data:', JSON.stringify(data));

        try {
            // Switch to the specified view if provided
            if (view && this.ui) {
                console.log('>>> [Explorer GLink] Switching to view:', view);
                this.ui.switchToSection(view);
            }

            // Handle specific actions (numeric action IDs)
            // Explorer Action IDs:
            // 1 = View Block
            // 2 = View Transaction
            // 3 = Search Domain
            // 4 = Search Query
            let actionExecuted = false;

            if (action !== undefined && action !== null) {
                console.log('>>> [Explorer GLink] Executing action:', action);
                switch (action) {
                    case 1: // View Block
                        if (data?.blockId) {
                            console.log('>>> [Explorer GLink] Action 1: Viewing block:', data.blockId);
                            this.viewBlockDetails(data.blockId);
                            actionExecuted = true;
                        }
                        break;

                    case 2: // View Transaction
                        if (data?.txId) {
                            console.log('>>> [Explorer GLink] Action 2: Viewing transaction:', data.txId);
                            this.viewTransactionDetails(data.txId);
                            actionExecuted = true;
                        }
                        break;

                    case 3: // Search Domain
                        if (data?.domain) {
                            console.log('>>> [Explorer GLink] Action 3: Searching domain:', data.domain);
                            this.searchDomain(data.domain);
                            actionExecuted = true;
                        }
                        break;

                    case 4: // Search Query
                        if (data?.query) {
                            console.log('>>> [Explorer GLink] Action 4: Search query:', data.query);
                            // Set the search input value
                            if (this.elements?.searchInput) {
                                this.elements.searchInput.value = data.query;
                            }
                            this.performSearch(data.query);
                            actionExecuted = true;
                        }
                        break;

                    default:
                        console.warn('>>> [Explorer GLink] Unknown action ID:', action);
                        glinkHandler.rejectGLink(`Unknown action ID: ${action}`);
                        return false;
                }
            }

            // Confirm successful processing
            if (actionExecuted) {
                console.log('>>> [Explorer GLink] Action executed successfully, confirming...');
                glinkHandler.confirmGLinkProcessed('Explorer: Action completed');
            } else if (view) {
                // View was switched but no specific action
                console.log('>>> [Explorer GLink] View switched, confirming...');
                glinkHandler.confirmGLinkProcessed('Explorer: View opened');
            } else {
                console.warn('>>> [Explorer GLink] No valid action or view specified');
                glinkHandler.rejectGLink('No valid action or view specified');
                return false;
            }

            console.log('>>> [Explorer GLink] processGLink completed successfully');
            return true;

        } catch (error) {
            console.error('>>> [Explorer GLink] Error processing GLink:', error);
            glinkHandler.rejectGLink(`Error: ${error.message}`);
            return false;
        }
    }

    // --- Explorer GLink Action IDs ---
    // 1 = View Block
    // 2 = View Transaction
    // 3 = Search Domain
    // 4 = Search Query

    /**
     * Create a GLink for Explorer with specific action and data
     * @param {number} action - Action ID (1=ViewBlock, 2=ViewTx, 3=SearchDomain, 4=Search)
     * @param {Object} data - Action data (e.g., {blockId}, {txId}, {domain}, {query})
     * @param {string} [view] - Optional view name
     * @returns {string} GLink URL
     * @static
     */
    static createGLink(action, data, view = null) {
        return CGLink.create(
            CUIBlockchainExplorer.getPackageID(),
            view,
            action,
            data
        );
    }

    /**
     * Create GLink for viewing a block
     * @param {string} blockId - Block ID
     * @returns {string} GLink URL
     */
    createBlockGLink(blockId) {
        return CUIBlockchainExplorer.createGLink(1, { blockId: blockId });
    }

    /**
     * Create GLink for viewing a transaction
     * @param {string} txId - Transaction ID
     * @returns {string} GLink URL
     */
    createTransactionGLink(txId) {
        return CUIBlockchainExplorer.createGLink(2, { txId: txId });
    }

    /**
     * Create GLink for viewing a domain
     * @param {string} domain - Domain name
     * @returns {string} GLink URL
     */
    createDomainGLink(domain) {
        return CUIBlockchainExplorer.createGLink(3, { domain: domain });
    }

    /**
     * Create GLink for a search query
     * @param {string} query - Search query
     * @returns {string} GLink URL
     */
    createSearchGLink(query) {
        return CUIBlockchainExplorer.createGLink(4, { query: query });
    }

    /**
     * Copy block GLink to clipboard
     * @param {string} blockId - Block ID
     * @returns {Promise<boolean>} True if successful
     */
    async copyBlockGLinkToClipboard(blockId) {
        const glink = this.createBlockGLink(blockId);
        const success = await CGLink.copyToClipboard(glink);
        if (success) {
            this.showNotification('Block share link copied to clipboard!');
        }
        return success;
    }

    /**
     * Copy transaction GLink to clipboard
     * @param {string} txId - Transaction ID
     * @returns {Promise<boolean>} True if successful
     */
    async copyTransactionGLinkToClipboard(txId) {
        const glink = this.createTransactionGLink(txId);
        const success = await CGLink.copyToClipboard(glink);
        if (success) {
            this.showNotification('Transaction share link copied to clipboard!');
        }
        return success;
    }

    /**
     * Copy domain GLink to clipboard
     * @param {string} domain - Domain name
     * @returns {Promise<boolean>} True if successful
     */
    async copyDomainGLinkToClipboard(domain) {
        const glink = this.createDomainGLink(domain);
        const success = await CGLink.copyToClipboard(glink);
        if (success) {
            this.showNotification('Domain share link copied to clipboard!');
        }
        return success;
    }

    /**
     * Copy search GLink to clipboard
     * @param {string} query - Search query
     * @returns {Promise<boolean>} True if successful
     */
    async copySearchGLinkToClipboard(query) {
        const glink = this.createSearchGLink(query);
        const success = await CGLink.copyToClipboard(glink);
        if (success) {
            this.showNotification('Search share link copied to clipboard!');
        }
        return success;
    }

    /**
     * Show notification to user
     * @param {string} message - Notification message
     */
    showNotification(message) {
        if (typeof Swal !== 'undefined') {
            Swal.fire({
                toast: true,
                position: 'top-end',
                icon: 'success',
                title: message,
                showConfirmButton: false,
                timer: 2500,
                timerProgressBar: true,
                customClass: {
                    popup: 'explorer-share-toast'
                }
            });
        } else {
            console.log('[Explorer] Notification:', message);
        }
    }

    // === GLink Support - END ===
}

/**
 * Data Manager - Handles API requests and data processing
 */
class BlockchainDataManager {
    /**
     * @param {Object} enums Blockchain enums
     * @param {Function} onBlockchainDataManager Callback for data updates
     * @param {Function} onErrorCallback Callback for errors
     */
    constructor(instance, enums, onUpdateCallback, onErrorCallback) {
        this.mInstance = instance;
        this.enums = enums;
        this.onUpdate = onUpdateCallback;
        this.onError = onErrorCallback;
        this.vmContext = CVMContext.getInstance();
        this.pendingRequests = {};
        this.requestIDs = {};
        this.mTools = instance.mTools;

        this.lastSortParams = {
            domains: {
                field: null,
                dir: null
            },
            transactions: {
                field: null,
                dir: null
            },
            blocks: {
                field: null,
                dir: null
            }
        };

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
        this.vmContext.addNewGridScriptResultListener(this.handleGridScriptResultCallback.bind(this), this.mID);
    }

    /**
     * Remove all event listeners
     */
    removeEventListeners() {
        if (!this.windowID)
            return;

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
     * Translates a request type code to a human-readable description
     * @param {number} requestType - Request type code from enums.eRequestType
     * @returns {string} Human-readable description of the request
     */
    getRequestTypeDescription(requestType) {
        const descriptions = {
            [this.enums.eRequestType.SEARCH]: "Blockchain Search",
            [this.enums.eRequestType.RECENT_BLOCKS]: "Recent Blocks",
            [this.enums.eRequestType.RECENT_TRANSACTIONS]: "Recent Transactions",
            [this.enums.eRequestType.BLOCK_DETAILS]: "Block Details",
            [this.enums.eRequestType.TRANSACTION_DETAILS]: "Transaction Details",
            [this.enums.eRequestType.DOMAIN_DETAILS]: "Domain Details",
            [this.enums.eRequestType.DOMAIN_HISTORY]: "Domain History",
            [this.enums.eRequestType.BLOCKCHAIN_STATUS]: "Blockchain Status",
            [this.enums.eRequestType.NETWORK_UTILIZATION]: "Network Utilization",
            [this.enums.eRequestType.BLOCK_SIZE]: "Block Size",
            [this.enums.eRequestType.BLOCK_REWARDS]: "Block Rewards",
            [this.enums.eRequestType.AVERAGE_BLOCK_TIME]: "Average Block Time",
            [this.enums.eRequestType.AVERAGE_KEY_BLOCK_TIME]: "Average Key Block Time",
            [this.enums.eRequestType.TRANSACTION_DAILY_STATS]: "Transaction Daily Stats",
            [this.enums.eRequestType.LIVENESS]: "Network Liveness",
            [this.enums.eRequestType.USDT_PRICE]: "USDT Price",
            [this.enums.eRequestType.HEIGHT]: "Blockchain Height",
            [this.enums.eRequestType.KEY_HEIGHT]: "Key Block Height",
            [this.enums.eRequestType.MARKET_CAP]: "Market Depth"
        };

        return descriptions[requestType] || `Unknown Request Type (${requestType})`;
    }

    /**
     * Create a pending request and set up timeout handling
     * @param {number} requestID Request ID
     * @param {number} type Request type from enums.eRequestType
     * @param {Object} additionalData Additional data to store with the request
     * @returns {Promise} Promise that resolves with the response or rejects on timeout
     */
    createPendingRequest(requestID, type, additionalData = {}) {
        return new Promise((resolve, reject) => {
            const requestTypeText = this.getRequestTypeDescription(type);
            const request = {
                resolve,
                reject,
                type,
                typeText: requestTypeText,
                ...additionalData,
                timestamp: Date.now()
            };

            this.pendingRequests[requestID] = request;

            // Set a timeout to reject the promise if no response is received
            const timeoutId = setTimeout(() => {
                if (this.pendingRequests[requestID]) {
                    delete this.pendingRequests[requestID];
                    const errorMessage = `Request timed out: ${requestTypeText} (ID: ${requestID})`;
                    console.error(`[Blockchain Explorer] ${errorMessage}`);
                    const error = new Error(errorMessage);
                    this.onError(error);
                    reject(error);
                }
            }, 5000); // 5 second timeout

            // Store the timeout ID so we can clear it if the request succeeds
            request.timeoutId = timeoutId;
        });
    }

    /**
     * Resolve a pending request
     * @param {number} requestID Request ID
     * @param {*} data Data to resolve the request with
     * @param {Object} [metadata] Additional metadata to return with the response
     */
    resolvePendingRequest(requestID, data, metadata = null) {
        const request = this.pendingRequests[requestID];
        if (!request)
            return;

        clearTimeout(request.timeoutId);

        // Include metadata with the resolved data if needed
        if (metadata) {
            request.resolve({
                data,
                metadata
            });
        } else {
            request.resolve(data);
        }

        delete this.pendingRequests[requestID];
    }

    /**
     * Reject a pending request
     * @param {number} requestID Request ID
     * @param {Error} error Error to reject the request with
     */
    rejectPendingRequest(requestID, error) {
        const request = this.pendingRequests[requestID];
        if (!request)
            return;

        clearTimeout(request.timeoutId);
        request.reject(error);
        delete this.pendingRequests[requestID];
    }

    // Requests Management - END

    /**
     * Request blockchain status
     * @returns {Promise<Object>} Blockchain status
     */
    async requestBlockchainStatus() {
        // API now returns the request ID directly
        const reqID = this.vmContext.getBlockchainStatus(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error('Failed to request blockchain status');
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
        }

        return this.createPendingRequest(reqID, this.enums.eRequestType.BLOCKCHAIN_STATUS);
    }

    /**
     * Requests market data from the blockchain
     * @param {number} page - Page number
     * @param {number} pageSize - Number of results per page
     * @param {number} sortType - Type of sorting (from eMarketSortType enum)
     * @param {number} sortOrder - Order of sorting (from eSortOrder enum)
     * @returns {Promise<Array>} - Processed market data
     */
    async requestMarketData(page = 1, pageSize = 20, sortType = 1, sortOrder = 1) {
        // Store the server-side sort parameters for synchronization
        this.lastSortParams = this.lastSortParams || {};
        this.lastSortParams.market = {
            type: sortType,
            order: sortOrder,
            clientParams: this.translateMarketSortToClient(sortType, sortOrder)
        };

        // Use the getMarketData method from vmContext
        const reqID = this.vmContext.getMarketData(
                false, // getMarketCap - we don't need total market cap for this view
                true, // getBalances - we want domain balances
                page,
                pageSize,
                sortType,
                sortOrder,
                this.getThreadID,
                this.mInstance,
                this.enums.eVMMetaCodeExecutionMode.RAW,
                0);
        if (!reqID) {
            throw new Error("Failed to request market data");
        }
        this.mInstance.addVMMetaRequestID(reqID);

        const results = await this.createPendingRequest(reqID, this.enums.eRequestType.MARKET_DATA, {
            page,
            size: pageSize,
            sortType,
            sortOrder
        });

        // Return both the results and the sort parameters for Tabulator
        return {
            data: results,
            sortParams: this.lastSortParams.market.clientParams
        };
    }

    /**
     * Requests total market depth.
     * @param {number} page - Page number
     * @param {number} pageSize - Number of results per page
     * @param {number} sortType - Type of sorting (from eMarketSortType enum)
     * @param {number} sortOrder - Order of sorting (from eSortOrder enum)
     * @returns {Promise<Array>} - Processed market data
     */
    async requestMarketDepth() {

        // Use the getMarketData method from vmContext
        const reqID = this.vmContext.getMarketData(
                true, // getMarketCap - need total market cap for this view
                false, // getBalances - we do NOT want domain balances
                undefined,
                undefined,
                undefined,
                undefined,
                this.getThreadID,
                this.mInstance,
                this.enums.eVMMetaCodeExecutionMode.RAW,
                0);
        if (!reqID) {
            throw new Error("Failed to request market data");
        }
        this.mInstance.addVMMetaRequestID(reqID);

        const results = await this.createPendingRequest(reqID, this.enums.eRequestType.MARKET_CAP);

        // Return both the results and the sort parameters for Tabulator
        return {
            data: results
        };
    }

    /**
     * Translates server-side market sort parameters to client-side Tabulator format
     * @param {number} sortType - Server sort type from enum
     * @param {number} sortOrder - Server sort order (0 for asc, 1 for desc)
     * @returns {Object} - Client sort parameters { field, dir }
     */
    translateMarketSortToClient(sortType, sortOrder) {
        // Map sort types to Tabulator field names using enum values
        const fieldMap = {
            [eMarketSortType.none]: null, // no sorting enabled
            [eMarketSortType.balance]: "balance", // Balance
            [eMarketSortType.received]: "txTotalReceived", // Total Received
            [eMarketSortType.sent]: "txTotalSent", // Total Sent
            [eMarketSortType.outgoing]: "txOutCount", // Outgoing Tx
            [eMarketSortType.incoming]: "txInCount", // Incoming Tx
            [eMarketSortType.total]: "txCount", // Total Tx
            [eMarketSortType.locked]: "lockedBalance", // Locked Assets
            [eMarketSortType.mined]: "GNCTotalMined" // Mined Assets
        };

        // Map sort order to Tabulator direction
        const dirMap = {
            0: "asc",
            1: "desc"
        };

        return {
            field: fieldMap[sortType] || "balance",
            dir: dirMap[sortOrder] || "desc"
        };
    }
    /**
     * Request network liveness
     * @returns {Promise<boolean>} Network liveness state
     */
    async requestLiveness() {
        const reqID = this.vmContext.getLiveness(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error('Failed to request blockchain liveness');
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
        }

        return this.createPendingRequest(reqID, this.enums.eRequestType.LIVENESS);
    }

    /**
     * Request USDT price
     * @returns {Promise<number>} USDT price
     */
    async requestUSDTPrice() {
        const reqID = this.vmContext.getUSDTPrice(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error('Failed to request USDT price');
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
        }

        return this.createPendingRequest(reqID, this.enums.eRequestType.USDT_PRICE);
    }

    /**
     * Request current blockchain height
     * @returns {Promise<Object>} Object containing regular and key heights
     */
    async requestHeight() {
        // Request regular height
        const heightReqID = this.vmContext.getHeight(
                false, this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!heightReqID) {
            throw new Error('Failed to request blockchain height');
        } else {
            this.mInstance.addVMMetaRequestID(heightReqID);
        }

        // Request key height in parallel
        const keyHeightReqID = this.vmContext.getHeight(
                true, this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!keyHeightReqID) {
            console.warn('Failed to request key height, proceeding with regular height only');
        } else {
            this.mInstance.addVMMetaRequestID(keyHeightReqID);
        }

        // Wait for both heights with separate pending requests for each
        const [height, keyHeight] = await Promise.all([
                    this.createPendingRequest(heightReqID, this.enums.eRequestType.HEIGHT),
                    keyHeightReqID ?
                    this.createPendingRequest(keyHeightReqID, this.enums.eRequestType.KEY_HEIGHT).catch(() => 0) :
                    Promise.resolve(0)
                ]);

        this.cache.currentHeight = height;
        this.cache.keyHeight = keyHeight;

        return {
            height,
            keyHeight
        };
    }
    /**
     * Request recent blocks
     * @param {number} size Number of blocks per page
     * @param {number} page Page number (1-based)
     * @param {number} sortBy Sort method (from enum)
     * @returns {Promise<Array>} Array of recent blocks
     */
    async requestRecentBlocks(size = 20, page = 1, sortBy = null) {
        // Use enum value for timestamp descending sort
        const sortBlocksByTimestampDesc =
            sortBy !== null ? sortBy :
            (this.enums.eSortBlocksBy.timestampDesc || 3); // Fallback to value 3 if enum not available

        const reqID = this.vmContext.getRecentBlocks(
                size, page, sortBlocksByTimestampDesc, this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error('Failed to request recent blocks');
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
        }

        const result = await this.createPendingRequest(reqID, this.enums.eRequestType.RECENT_BLOCKS, {
            size,
            page,
            sortBy
        });

        let blocks;
        let pagination = null;

        // Check if we received an object with both data and metadata
        if (result && result.data && result.metadata) {
            blocks = result.data;
            pagination = result.metadata.pagination;
        } else {
            blocks = result;
        }

        // Update cache
        this.cache.recentBlocks = blocks;

        // If we didn't get pagination info, but we know we're paginating, create some estimates
        if (!pagination && page > 0) {
            pagination = {
                currentPage: page,
                pageSize: size,
                totalItems: blocks.length > 0 ? blocks[0].height || blocks.length * 10 : 100, // Estimate from block height or just a placeholder
                totalPages: Math.ceil((blocks.length > 0 ? blocks[0].height || blocks.length * 10 : 100) / size)
            };
        }

        // Call update with both data and pagination metadata
        if (this.onUpdate) {
            this.onUpdate(this.enums.eRequestType.RECENT_BLOCKS, blocks, {
                pagination
            }).catch(error => console.error("Error updating UI:", error));
        }

        return blocks;
    }

    /**
     * Request recent transactions
     * @param {number} size Number of transactions per page
     * @param {number} page Page number (1-based)
     * @param {boolean} includeMem Include mempool transactions
     * @returns {Promise<Array>} Array of recent transactions
     */
    async requestRecentTransactions(size = 20, page = 1, includeMem = false) {
        // Note: API expects includeMem parameter (boolean) before threadID
        const reqID = this.vmContext.getRecentTransactions(
                size, page, includeMem, this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error('Failed to request recent transactions');
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
        }

        const result = await this.createPendingRequest(reqID, this.enums.eRequestType.RECENT_TRANSACTIONS, {
            size,
            page,
            includeMem
        });

        let transactions;
        let pagination = null;

        // Check if we received an object with both data and metadata
        if (result && result.data && result.metadata) {
            transactions = result.data;
            pagination = result.metadata.pagination;
        } else {
            transactions = result;
        }

        // Update cache
        this.cache.recentTransactions = transactions;

        // If we didn't get pagination info, but we know we're paginating, create some estimates
        if (!pagination && page > 0) {
            pagination = {
                currentPage: page,
                pageSize: size,
                totalItems: transactions.length * Math.max(page, 5), // Rough estimate
                totalPages: Math.max(page, 5)
            };
        }

        // Call update with both data and pagination metadata
        if (this.onUpdate) {
            this.onUpdate(this.enums.eRequestType.RECENT_TRANSACTIONS, transactions, {
                pagination
            }).catch(error => console.error("Error updating UI:", error));
        }

        return transactions;
    }

    /**
     * Request transaction daily statistics
     * @param {number} days Number of days to request stats for
     * @returns {Promise<Array>} Array of daily statistics
     */
    async requestTransactionDailyStats(days = 14) {
        // Note: API expects includeMem parameter
        const includeMem = false; // Default value

        const reqID = this.vmContext.getTransactionDailyStats(
                days, includeMem, this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error('Failed to request transaction daily stats');
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
        }

        const stats = await this.createPendingRequest(reqID, this.enums.eRequestType.TRANSACTION_DAILY_STATS, {
            days
        });
        this.cache.transactionDailyStats = stats;
        return stats;
    }

    // Non-Blocking Calls - BEGIN

    /**
     * Request network utilization (24h)
     * @returns {Promise<number>} Network utilization percentage
     */
    async requestNetworkUtilization() {
        const reqID = this.vmContext.getNetworkUtilization24h(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error('Failed to request network utilization');
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
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
        const reqID = this.vmContext.getBlockSize24h(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error('Failed to request average block size');
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
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
        const reqID = this.vmContext.getBlockRewards24h(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error('Failed to request block rewards');
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
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
        const reqID = this.vmContext.getAverageBlockTime24h(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error('Failed to request average block time');
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
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
        const reqID = this.vmContext.getAverageKeyBlockTime24h(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error('Failed to request average key block time');
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
        }

        const keyBlockTime = await this.createPendingRequest(reqID, this.enums.eRequestType.AVERAGE_KEY_BLOCK_TIME);
        this.cache.avgKeyBlockTime = keyBlockTime;
        return keyBlockTime;
    }
    // Non-Blocking Calls API Usage- END

    // Blocking Calls API Usage - BEGIN
    /**
     * Request network utilization (24h)
     * @returns {Promise<number>} Network utilization percentage
     */
    async getNetworkUtilization() {
        const utilization = await this.vmContext.getNetworkUtilization24hA(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        this.cache.networkUtilization = utilization;
        return utilization;
    }

    /**
     * Request average block size (24h)
     * @returns {Promise<number>} Average block size in bytes
     */
    async getBlockSize() {
        const blockSize = await this.vmContext.getBlockSize24hA(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        this.cache.blockSize = blockSize;
        return blockSize;
    }

    /**
     * Request average block rewards (24h)
     * @returns {Promise<number>} Average block rewards
     */
    async getBlockRewards() {
        const blockRewards = await this.vmContext.getBlockRewards24h(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        this.cache.blockRewards = blockRewards;
        return blockRewards;
    }

    /**
     * Request average block time (24h)
     * @returns {Promise<number>} Average block time in seconds
     */
    async getAverageBlockTime() {
        const blockTime = await this.vmContext.getAverageBlockTime24hA(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        this.cache.avgBlockTime = blockTime;
        return blockTime;
    }

    /**
     * Request average key block time (24h)
     * @returns {Promise<number>} Average key block time in seconds
     */
    async getAverageKeyBlockTime() {
        const keyBlockTime = await this.vmContext.getAverageKeyBlockTime24hA(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        this.cache.avgKeyBlockTime = keyBlockTime;
        return keyBlockTime;
    }
    // Blocking Calls API Usage- END

    /**
     * Request details for a specific block
     * @param {string} blockID Block ID
     * @returns {Promise<Object>} Block details
     */
    async requestBlockDetails(blockID) {
        // Note: API expects includeTX, includeMem, includeSec parameters
        const includeTX = true;
        const includeMem = false;
        const includeSec = false;

        const reqID = this.vmContext.getBlockDetails(
                blockID, includeTX, includeMem, includeSec, this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error(`Failed to request details for block: ${blockID}`);
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
        }

        return this.createPendingRequest(reqID, this.enums.eRequestType.BLOCK_DETAILS, {
            blockID
        });
    }

    /**
     * Request details for a specific transaction
     * @param {string} transactionID Transaction ID
     * @returns {Promise<Object>} Transaction details
     */
    async requestTransactionDetails(transactionID) {
        const reqID = this.vmContext.getTransactionDetails(
                transactionID, this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error(`Failed to request details for transaction: ${transactionID}`);
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
        }

        return this.createPendingRequest(reqID, this.enums.eRequestType.TRANSACTION_DETAILS, {
            transactionID
        });
    }

    /**
     * Request details for a domain
     * @param {string} address Domain address
     * @returns {Promise<Object>} Domain details
     */
    async requestDomainDetails(address) {
        // Note: API expects perspective and includeSec parameters
        const perspective = "";
        const includeSec = false;

        const reqID = this.vmContext.getDomainDetails(
                address, perspective, includeSec, this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error(`Failed to request details for domain: ${address}`);
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
        }

        return this.createPendingRequest(reqID, this.enums.eRequestType.DOMAIN_DETAILS, {
            address
        });
    }

    /**
     * Translates server-side sort parameters to client-side Tabulator format
     * @param {number} serverSortType - Server sort type from enum
     * @param {number} sortOrder - Server sort order (0 for asc, 1 for desc) - optional
     * @returns {Object} - Client sort parameters { field, dir }
     */
    translateServerSortToClient(serverSortType, sortOrder) {
        // Direction mapping
        const dirMap = {
            0: "asc",
            1: "desc"
        };

        // Check if this is a transaction sort type
        if (this.enums.eSortTransactionsBy && Object.values(this.enums.eSortTransactionsBy).includes(serverSortType)) {
            const transactionSortMappings = {
                [this.enums.eSortTransactionsBy.heightAsc]: {
                    field: "height",
                    dir: "asc"
                },
                [this.enums.eSortTransactionsBy.heightDesc]: {
                    field: "height",
                    dir: "desc"
                },
                [this.enums.eSortTransactionsBy.timestampAsc]: {
                    field: "time",
                    dir: "asc"
                },
                [this.enums.eSortTransactionsBy.timestampDesc]: {
                    field: "time",
                    dir: "desc"
                },
                [this.enums.eSortTransactionsBy.valueAsc]: {
                    field: "value",
                    dir: "asc"
                },
                [this.enums.eSortTransactionsBy.valueDesc]: {
                    field: "value",
                    dir: "desc"
                }
            };

            return transactionSortMappings[serverSortType] || {
                field: "time",
                dir: "desc"
            };
        }

        // Handle market data sort types from eMarketSortType enum
        // Map sort types to Tabulator field names based on enum values
        const marketFieldMap = {
            [eMarketSortType.none]: null, // none - no sorting
            [eMarketSortType.balance]: "balance", // balance
            [eMarketSortType.received]: "txTotalReceived", // received
            [eMarketSortType.sent]: "txTotalSent", // sent
            [eMarketSortType.outgoing]: "txOutCount", // outgoing
            [eMarketSortType.incoming]: "txInCount", // incoming
            [eMarketSortType.total]: "txCount", // total
            [eMarketSortType.locked]: "lockedBalance", // locked
            [eMarketSortType.mined]: "GNCTotalMined" // Mined Assets

        };

        // For market sorts, we need the direction as a parameter
        const dir = sortOrder !== undefined ? dirMap[sortOrder] || "desc" : "desc";
        const field = marketFieldMap[serverSortType];

        // Return null field if sort type is "none", otherwise return the appropriate field and direction
        return {
            field: field,
            dir: dir
        };
    }

    /**
     * Request transaction history for a domain
     * @param {string} address Domain address
     * @param {number} size Number of transactions per page
     * @param {number} page Page number (1-based)
     * @param {number} sortBy Sort method (from enum)
     * @param {string} stateID State ID for restoring iterators
     * @returns {Promise<Array>} Domain transaction history
     */
    async requestDomainHistory(address, size = 20, page = 1, sortBy = null, stateID = "") {

        const sortType = null !== sortBy ? sortBy : this.enums.eSortTransactionsBy.timestampDesc;
        this.lastSortParams.domains = this.translateServerSortToClient(sortType);

        // Use enum value for timestamp descending sort or fallback to 3
        const sortByTimestampDesc =
            sortBy !== null ? sortBy :
            (this.enums.eSortTransactionsBy.timestampDesc || 3);

        const reqID = this.vmContext.getDomainHistory(
                address, size, page, sortByTimestampDesc, stateID, this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error(`Failed to request history for domain: ${address}`);
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
        }

        const result = await this.createPendingRequest(reqID, this.enums.eRequestType.DOMAIN_HISTORY, {
            address,
            size,
            page
        });

        let transactions;
        let pagination = null;

        // Check if we received an object with both data and metadata
        if (result && result.data && result.metadata) {
            transactions = result.data;
            pagination = result.metadata.pagination;
        } else {
            transactions = result;
        }

        // If we didn't get pagination info, create an estimate
        if (!pagination) {
            pagination = {
                currentPage: page,
                pageSize: size,
                totalItems: transactions.length * Math.max(page, 3), // Rough estimate
                totalPages: Math.max(page, 3)
            };
        }

        // Call update with both data and pagination metadata
        if (this.onUpdate) {
            this.onUpdate(this.enums.eRequestType.DOMAIN_HISTORY, transactions, {
                pagination
            }).catch(error => console.error("Error updating UI:", error));
        }

        return transactions;
    }

    /**
     * Search the blockchain with a query string
     * @param {string} query Query string
     * @param {CSearchFilter|null} searchFilter Optional search filter
     * @param {number} size Number of results per page
     * @param {number} page Page number (1-based)
     * @returns {Promise<Array>} Search results
     */
    async searchBlockchain(query, searchFilter = null, size = 10, page = 1) {
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

        const reqID = this.vmContext.searchBlockchain(
                query, size, page, searchFilter, this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            throw new Error(`Failed to search for "${query}"`);
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
        }

        this.lastSearchQuery = query;

        const result = await this.createPendingRequest(reqID, this.enums.eRequestType.SEARCH, {
            query,
            size,
            page
        });

        let searchResults;
        let pagination = null;

        // Check if we received an object with both data and metadata
        if (result && result.data && result.metadata) {
            searchResults = result.data;
            pagination = result.metadata.pagination;
        } else {
            searchResults = result;
        }

        // If we didn't get pagination info, create an estimate
        if (!pagination) {
            pagination = {
                currentPage: page,
                pageSize: size,
                totalItems: searchResults.length * Math.max(page, 5), // Rough estimate
                totalPages: Math.max(page, 5)
            };
        }

        // Call update with both data and pagination metadata
        if (this.onUpdate) {
            this.onUpdate(this.enums.eRequestType.SEARCH, searchResults, {
                pagination
            }).catch(error => console.error("Error updating UI:", error));
        }

        return searchResults;
    }

    /**
     * Subscribe to blockchain updates
     * @returns {boolean} Success status
     */
    subscribeToBlockchainUpdates() {
        const reqID = this.vmContext.subscribeToBlockchainUpdates(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            return false;
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
            return true;
        }
    }

    /**
     * Unsubscribe from blockchain updates
     * @returns {boolean} Success status
     */
    unsubscribeFromBlockchainUpdates() {
        const reqID = this.vmContext.unsubscribeFromBlockchainUpdates(
                this.getThreadID, this.mInstance, this.enums.eVMMetaCodeExecutionMode.RAW, 0);

        if (!reqID) {
            return false;
        } else {
            this.mInstance.addVMMetaRequestID(reqID);
            return true;
        }
    }

    // Event handlers for blockchain data

    /**
     * Process domain information from search results
     * @param {CSearchResults} results - Search results containing domain descriptions
     * @returns {Array} - Array of processed domain objects
     */
    processDomainsFromSearchResults(results) {
        if (!(results instanceof CSearchResults)) {
            console.warn("Received object is not a CSearchResults instance");
            return [];
        }

        const domains = [];
        const resultCount = results.getResultCount();

        for (let i = 0; i < resultCount; i++) {
            try {
                const result = results.getResult(i).getValue();
                if (result instanceof CDomainDesc ||
                    (result.constructor && result.constructor.name === "CDomainDesc")) {
                    domains.push(result);
                }
            } catch (error) {
                console.error("Error processing domain from search results:", error);
            }
        }

        return domains;
    }

    /**
     * Process blocks from CSearchResults with enhanced error handling
     * @param {CSearchResults} searchResults Search results object
     * @returns {Object} Processed blocks with success/error information
     */
    processBlocksFromSearchResults(searchResults) {

        let blocks = [];
        try {
            // First, verify we have a valid CSearchResults instance
            if (!(searchResults instanceof CSearchResults)) {
                return [];
            }

            const blocks = [];
            const count = searchResults.getResultCount();

            // Extract all block results
            for (let i = 0; i < count; i++) {
                try {
                    const resultItem = searchResults.getResult(i).getValue();

                    if (resultItem instanceof CBlockDesc) {
                        blocks.push(resultItem);
                    }
                } catch (error) {
                    console.error("Error processing block from search results:", error);
                    return [];
                    // Continue processing other results despite this error
                }
            }

            return blocks;
        } catch (error) {
            console.error("Fatal error processing blocks from search results:", error);
            return [];
        }
    }

    /**
     * Process transactions from CSearchResults
     * @param {CSearchResults} searchResults Search results object
     * @returns {Array} Processed transactions
     */
    processTransactionsFromSearchResults(searchResults) {

        // First, verify we have a valid CSearchResults instance
        if (!(searchResults instanceof CSearchResults)) {
            console.warn("Received object is not a CSearchResults instance");
            return [];
        }

        const transactions = [];
        const count = searchResults.getResultCount();

        // Extract all transaction results
        for (let i = 0; i < count; i++) {
            try {
                const resultItem = searchResults.getResult(i).getValue();

                if (resultItem instanceof CTransactionDesc) {
                    transactions.push(resultItem);
                }
            } catch (error) {
                console.error("Error processing transaction from search results:", error);
            }
        }

        return transactions;
    }

    /**
     * Process domain history from CSearchResults
     * @param {CSearchResults} searchResults Search results object
     * @param {string} domainAddress Domain address
     * @returns {Array} Processed domain history
     */
    processDomainHistoryFromSearchResults(searchResults, domainAddress) {
        // First, verify we have a valid CSearchResults instance
        if (!(searchResults instanceof CSearchResults)) {
            console.warn("Received object is not a CSearchResults instance");
            return [];
        }

        const transactions = [];
        const count = searchResults.getResultCount();

        // Extract all transaction results for this domain
        for (let i = 0; i < count; i++) {
            try {
                const resultItem = searchResults.getResult(i).getValue();

                if (resultItem instanceof CTransactionDesc) {
                    const processedTx = this.mTools.convertBlockchainMetaToPlainObject(resultItem); // so that we can view this object with Tabulator.js
                    // fied names need to match (no 'm' prefix) and the object needs to be mutable.

                    // Determine if this is an incoming or outgoing transaction
                    let type = 'Transfer';
                    let counterparty = '';

                    if (processedTx.sender === domainAddress) {
                        type = 'Outgoing';
                        counterparty = processedTx.receiver;
                    } else if (processedTx.receiver === domainAddress) {
                        type = 'Incoming';
                        counterparty = processedTx.sender;
                    }

                    transactions.push({
                        ...processedTx, // spread exisitng flat object onto the new one (shallow copy all exisitng fields)
                        type, // once done - add additoinal high level knowledge.
                        counterparty
                    });
                }
            } catch (error) {
                console.error("Error processing domain history from search results:", error);
            }
        }

        return transactions;
    }
    // Blockchain Explorer API Data Handlers - BEGIN
    /**
     * Process CSearchResults object
     * @param {CSearchResults} searchResults Search results object
     * @returns {Array} Processed search results
     */
    /**
     * Process CSearchResults object
     * @param {CSearchResults} searchResults Search results object
     * @returns {Array} Processed search results
     */
    processSearchResults(searchResults) {
        // First, verify we have a valid CSearchResults instance
        if (!(searchResults instanceof CSearchResults)) {
            console.warn("Received object is not a CSearchResults instance");
            return [];
        }

        // Initialize processed results array
        const processedResults = [];

        // Get the number of results from the CSearchResults object
        const resultCount = searchResults.getResultCount();

        if (resultCount > 0) {
            for (let i = 0; i < resultCount; i++) {
                try {
                    // Get the result using the proper method
                    const resultItem = searchResults.getResult(i).getValue();

                    if (resultItem instanceof CBlockDesc) {

                        processedResults.push(resultItem);
                        // processedResults.push({
                        // Include the block data directly
                        //data: resultItem
                        //  resultType: this.enums.eSearchResultElemType.BLOCK
                        // });
                    } else if (resultItem instanceof CTransactionDesc) {
                        processedResults.push(resultItem);
                        // processedResults.push({
                        // Include the transaction data directly
                        //	data: resultItem
                        //   resultType: this.enums.eSearchResultElemType.TRANSACTION
                        //});
                    } else if (resultItem instanceof CDomainDesc) {
                        processedResults.push(resultItem);
                        ///processedResults.push({
                        // Include the domain data directly
                        //data: resultItem
                        // resultType: this.enums.eSearchResultElemType.DOMAIN
                        // });
                    }
                } catch (error) {
                    console.error("Error processing search result:", error);
                }
            }
        }

        return processedResults;
    }
    /**
     * Handler for search results events
     * @param {Object} arg Event argument containing search results
     */
    handleSearchResults(arg) {
        if (!arg || !arg.results)
            return;

        const {
            results,
            reqID
        } = arg;
        const pendingRequest = this.pendingRequests[reqID];

        if (!pendingRequest)
            return;

        try {
            // Verify the object is a CSearchResults instance
            if (!(results instanceof CSearchResults)) {
                throw new Error("Received object is not a CSearchResults instance");
            }

            // Process the results based on the request type
            let processedResults;
            let paginationMetadata = null;

            // Default page sizes
            const DEFAULT_SEARCH_PAGE_SIZE = 10;
            const DEFAULT_BLOCKS_PAGE_SIZE = 20;
            const DEFAULT_TRANSACTIONS_PAGE_SIZE = 20;
            const DEFAULT_DOMAIN_HISTORY_PAGE_SIZE = 20;

            // Estimation multipliers for total items
            const SEARCH_RESULTS_MULTIPLIER = 2;
            const BLOCKS_HEIGHT_FALLBACK_MULTIPLIER = 10;
            const TRANSACTIONS_COUNT_MULTIPLIER = 5;
            const DOMAIN_HISTORY_MULTIPLIER = 3;

            // Minimum page counts for estimation
            const MIN_TRANSACTIONS_PAGES = 5;
            const MIN_DOMAIN_HISTORY_PAGES = 3;

            // Default estimated total for blocks when no data
            const DEFAULT_BLOCKS_TOTAL = 100;

            switch (pendingRequest.type) {
            case this.enums.eRequestType.SEARCH:
                processedResults = this.processSearchResults(results);

                // Use CSearchResults methods if available
                if (results instanceof CSearchResults) {
                    paginationMetadata = {
                        currentPage: results.getCurrentPage() || pendingRequest.page || 1,
                        pageSize: results.getItemsPerPage() || pendingRequest.size || DEFAULT_SEARCH_PAGE_SIZE,
                        totalItems: results.getTotalResultCount() || processedResults.length,
                        totalPages: Math.ceil((results.getTotalResultCount() || processedResults.length) /
                            (results.getItemsPerPage() || pendingRequest.size || DEFAULT_SEARCH_PAGE_SIZE))
                    };
                } else if (pendingRequest.page) {
                    // Fall back to estimation if we know we're paginating
                    paginationMetadata = {
                        currentPage: pendingRequest.page,
                        pageSize: pendingRequest.size || DEFAULT_SEARCH_PAGE_SIZE,
                        totalPages: Math.max(pendingRequest.page, Math.ceil(processedResults.length * SEARCH_RESULTS_MULTIPLIER /
                                (pendingRequest.size || DEFAULT_SEARCH_PAGE_SIZE))),
                        totalItems: processedResults.length * SEARCH_RESULTS_MULTIPLIER
                    };
                }
                break;

            case this.enums.eRequestType.RECENT_BLOCKS:
                processedResults = this.processBlocksFromSearchResults(results);
                this.cache.recentBlocks = processedResults;

                // Use CSearchResults methods if available
                if (results instanceof CSearchResults) {
                    const estimatedTotal = results.getTotalResultCount() ||
                        (processedResults.length > 0 ?
                            processedResults[0].height || processedResults.length * BLOCKS_HEIGHT_FALLBACK_MULTIPLIER :
                            DEFAULT_BLOCKS_TOTAL);

                    paginationMetadata = {
                        currentPage: results.getCurrentPage() || pendingRequest.page || 1,
                        pageSize: results.getItemsPerPage() || pendingRequest.size || DEFAULT_BLOCKS_PAGE_SIZE,
                        totalItems: estimatedTotal,
                        totalPages: Math.ceil(estimatedTotal / (results.getItemsPerPage() || pendingRequest.size || DEFAULT_BLOCKS_PAGE_SIZE))
                    };
                } else if (pendingRequest.page) {
                    const estimatedTotal = processedResults.length > 0 ?
                        processedResults[0].height || processedResults.length * BLOCKS_HEIGHT_FALLBACK_MULTIPLIER :
                        DEFAULT_BLOCKS_TOTAL;

                    paginationMetadata = {
                        currentPage: pendingRequest.page,
                        pageSize: pendingRequest.size || DEFAULT_BLOCKS_PAGE_SIZE,
                        totalPages: Math.ceil(estimatedTotal / (pendingRequest.size || DEFAULT_BLOCKS_PAGE_SIZE)),
                        totalItems: estimatedTotal
                    };
                }
                break;

            case this.enums.eRequestType.RECENT_TRANSACTIONS:
                processedResults = this.processTransactionsFromSearchResults(results);
                this.cache.recentTransactions = processedResults;

                // Use CSearchResults methods if available
                if (results instanceof CSearchResults) {
                    const totalItems = results.getTotalResultCount() ||
                        processedResults.length * TRANSACTIONS_COUNT_MULTIPLIER;

                    paginationMetadata = {
                        currentPage: results.getCurrentPage() || pendingRequest.page || 1,
                        pageSize: results.getItemsPerPage() || pendingRequest.size || DEFAULT_TRANSACTIONS_PAGE_SIZE,
                        totalItems: totalItems,
                        totalPages: Math.ceil(totalItems / (results.getItemsPerPage() || pendingRequest.size || DEFAULT_TRANSACTIONS_PAGE_SIZE))
                    };
                } else if (pendingRequest.page) {
                    paginationMetadata = {
                        currentPage: pendingRequest.page,
                        pageSize: pendingRequest.size || DEFAULT_TRANSACTIONS_PAGE_SIZE,
                        totalPages: Math.max(pendingRequest.page, MIN_TRANSACTIONS_PAGES),
                        totalItems: processedResults.length * TRANSACTIONS_COUNT_MULTIPLIER
                    };
                }
                break;

            case this.enums.eRequestType.MARKET_DATA:
                // Market data comes as a list of domain descriptions in search results format
                processedResults = this.processDomainsFromSearchResults(results);
                // Use CSearchResults methods if available
                if (results instanceof CSearchResults) {
                    const totalItems = results.getTotalResultCount() ||
                        processedResults.length * DOMAIN_HISTORY_MULTIPLIER;
                    paginationMetadata = {
                        currentPage: results.getCurrentPage() || pendingRequest.page || 1,
                        pageSize: results.getItemsPerPage() || pendingRequest.size || DEFAULT_DOMAIN_HISTORY_PAGE_SIZE,
                        totalItems: totalItems,
                        totalPages: Math.ceil(totalItems / (results.getItemsPerPage() || pendingRequest.size || DEFAULT_DOMAIN_HISTORY_PAGE_SIZE))
                    };
                } else if (pendingRequest.page) {
                    paginationMetadata = {
                        currentPage: pendingRequest.page,
                        pageSize: pendingRequest.size || DEFAULT_DOMAIN_HISTORY_PAGE_SIZE,
                        totalPages: Math.max(pendingRequest.page, MIN_DOMAIN_HISTORY_PAGES),
                        totalItems: processedResults.length * DOMAIN_HISTORY_MULTIPLIER
                    };
                }
                break;

            case this.enums.eRequestType.DOMAIN_HISTORY:
                processedResults = this.processDomainHistoryFromSearchResults(results, pendingRequest.address);

                // Use CSearchResults methods if available
                if (results instanceof CSearchResults) {
                    const totalItems = results.getTotalResultCount() ||
                        processedResults.length * DOMAIN_HISTORY_MULTIPLIER;

                    paginationMetadata = {
                        currentPage: results.getCurrentPage() || pendingRequest.page || 1,
                        pageSize: results.getItemsPerPage() || pendingRequest.size || DEFAULT_DOMAIN_HISTORY_PAGE_SIZE,
                        totalItems: totalItems,
                        totalPages: Math.ceil(totalItems / (results.getItemsPerPage() || pendingRequest.size || DEFAULT_DOMAIN_HISTORY_PAGE_SIZE))
                    };
                } else if (pendingRequest.page) {
                    paginationMetadata = {
                        currentPage: pendingRequest.page,
                        pageSize: pendingRequest.size || DEFAULT_DOMAIN_HISTORY_PAGE_SIZE,
                        totalPages: Math.max(pendingRequest.page, MIN_DOMAIN_HISTORY_PAGES),
                        totalItems: processedResults.length * DOMAIN_HISTORY_MULTIPLIER
                    };
                }
                break;

            default:
                console.warn(`Unknown request type for search results: ${pendingRequest.type}`);
                processedResults = this.vmContext.processSearchResults(results);
            }

            // Update the relevant UI sections through the callback
            if (this.onUpdate) {
                const metadata = paginationMetadata ? {
                    pagination: paginationMetadata
                }
                 : null;
                this.onUpdate(pendingRequest.type, processedResults, metadata).catch(error => console.error("Error updating UI:", error));
            }

            // Resolve the pending request with both data and metadata if available
            if (paginationMetadata) {
                this.resolvePendingRequest(reqID, {
                    data: processedResults,
                    metadata: {
                        pagination: paginationMetadata
                    }
                });
            } else {
                this.resolvePendingRequest(reqID, processedResults);
            }
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
        if (!arg || !arg.data)
            return;

        const {
            data,
            reqID
        } = arg;
        const pendingRequest = this.pendingRequests[reqID];

        if (!pendingRequest)
            return;

        try {
            // Verify the object is a CBlockDesc instance
            if (!(data instanceof CBlockDesc || (data.constructor && data.constructor.name === 'CBlockDesc'))) {
                throw new Error("Received object is not a CBlockDesc instance");
            }

            // Update the UI through the callback
            if (this.onUpdate) {
                this.onUpdate(pendingRequest.type, data).catch(error => console.error("Error updating UI:", error));
            }

            // Resolve the pending request
            this.resolvePendingRequest(reqID, data);
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

    async handleGridScriptResultCallback(result) {
        if (result == null)
            return;
        // [ Notice ]: invocation of any GridScritp command would cause GRIDNTE OS to inject request identitifer into this process.
        //			   Whether the request was an explicit GridScript command issuance or implicit through a high-level API call such as one of Blockchain Explorer API set.
        if (this.mInstance.hasVMMetaRequestID(result.reqID)) { // the vm-operation indeed was triggered by our application
            if (result.explicitTypes) { // we're interested only in notification which contains strongly typed data

                // Check if both types and data are arrays with the same length
                if (Array.isArray(result.types) && Array.isArray(result.data) && result.types.length === result.data.length) {
                    // Process each pair of type and data
                    for (let i = 0; i < result.types.length; i++) {
                        this.handleBlockchainStats({
                            type: result.types[i], // Important: this is a strongly typed simple data type
                            data: result.data[i], // <- this JavaScipt loosly typed value was derived from.
                            reqID: result.reqID
                        });
                    }
                } else {
                    // Process as single values (non-array or mismatched arrays)
                    this.handleBlockchainStats({
                        type: result.types, // Important: this is a strongly typed simple data type
                        data: result.data, // <- this JavaScipt loosly typed value was derived from.
                        reqID: result.reqID
                    });
                }
            }
        }
    }

    /**
     * Handler for domain details events
     * @param {Object} arg Event argument containing domain details
     */
    handleDomainDetails(arg) {
        if (!arg || !arg.data)
            return;

        const {
            data,
            reqID
        } = arg;
        const pendingRequest = this.pendingRequests[reqID];

        if (!pendingRequest)
            return;

        try {
            // Verify the object is a CDomainDesc instance
            if (!(data instanceof CDomainDesc || (data.constructor && data.constructor.name === 'CDomainDesc'))) {
                throw new Error("Received object is not a CDomainDesc instance");
            }

            // Update the UI through the callback
            if (this.onUpdate) {
                this.onUpdate(pendingRequest.type, data).catch(error => console.error("Error updating UI:", error));
            }

            // Resolve the pending request
            this.resolvePendingRequest(reqID, data);
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
        if (!arg || !arg.data)
            return;

        const {
            transactionDesc,
            reqID
        } = arg;
        const pendingRequest = this.pendingRequests[reqID];

        if (!pendingRequest)
            return;

        try {
            // Verify the object is a CTransactionDesc instance
            if (!(transactionDesc instanceof CTransactionDesc || (transactionDesc.constructor && transactionDesc.constructor.name === 'CTransactionDesc'))) {
                throw new Error("Received object is not a CTransactionDesc instance");
            }

            // Update the UI through the callback
            if (this.onUpdate) {
                this.onUpdate(pendingRequest.type, transactionDesc).catch(error => console.error("Error updating UI:", error));
            }

            // Resolve the pending request
            this.resolvePendingRequest(reqID, transactionDesc);
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
        if (!arg || !arg.data)
            return;

        const {
            type,
            data,
            reqID
        } = arg;
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

                case this.enums.eRequestType.MARKET_CAP:

                    if (data.marketCap) {
                        processedData = data.marketCap;
                        this.cache.marketCap = data.marketCap;
                    }

                    break;

                case this.enums.eRequestType.MARKET_DATA:

                    if (data.marketCap) {
                        this.cache.marketCap = marketCap;
                    }

                    break;

                case this.enums.eRequestType.KEY_HEIGHT:
                    processedData = data.height || data;
                    this.cache.keyHeight = processedData;
                    break;

                }

                // Update the UI through the callback
                if (this.onUpdate) {
                    this.onUpdate(requestType, processedData).catch(error => console.error("Error updating UI:", error));
                }

                // Resolve the pending request
                this.resolvePendingRequest(reqID, processedData);
            } else {
                // This might be a subscription update, so update our cache based on the stats type
                if (type === 'blockchainStatus') {
                    this.cache.blockchainStatus = data;
                    if (this.onUpdate)
                        this.onUpdate(this.enums.eRequestType.BLOCKCHAIN_STATUS, data).catch(error => console.error("Error updating UI:", error));
                } else if (type === 'blockchainHeight') {
                    if (data.keyHeight) {
                        this.cache.keyHeight = data.keyHeight;
                        if (this.onUpdate)
                            this.onUpdate(this.enums.eRequestType.KEY_HEIGHT, data.keyHeight).catch(error => console.error("Error updating UI:", error));
                    }
                    if (data.height) {
                        this.cache.currentHeight = data.height;
                        if (this.onUpdate)
                            this.onUpdate(this.enums.eRequestType.HEIGHT, data.height).catch(error => console.error("Error updating UI:", error));
                    }
                } else if (type === 'usdtPrice') {
                    this.cache.usdtPrice = data.price || data;
                    if (this.onUpdate)
                        this.onUpdate(this.enums.eRequestType.USDT_PRICE, data.price || data);
                } else if (type === 'livenessInfo') {
                    this.cache.liveness = data.state || data;
                    if (this.onUpdate)
                        this.onUpdate(this.enums.eRequestType.LIVENESS, data.state || data).catch(error => console.error("Error updating UI:", error));
                } else if (type === 'networkUtilization') {
                    this.cache.networkUtilization = data.utilization || data;
                    if (this.onUpdate)
                        this.onUpdate(this.enums.eRequestType.NETWORK_UTILIZATION, data.utilization || data).catch(error => console.error("Error updating UI:", error));
                } else if (type === 'marketDepth') {
                    this.cache.marketDepth = data.marketCap || data;
                    if (this.onUpdate)
                        this.onUpdate(this.enums.eRequestType.MARKET_CAP, data.marketCap || data).catch(error => console.error("Error updating UI:", error));
                } else if (type === 'transactionDailyStats') {
                    this.cache.transactionDailyStats = data;
                    if (this.onUpdate)
                        this.onUpdate(this.enums.eRequestType.TRANSACTION_DAILY_STATS, data).catch(error => console.error("Error updating UI:", error));
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
     * @param {Object} paginationState Pagination state object
     */
    constructor(instance, elements, enums, paginationState) {
        this.mInstance = instance;
        this.mTools = this.mInstance.mTools;
        this.elements = elements;
        this.enums = enums;
        this.paginationState = paginationState;

        // Table instances
        this.tables = {};

        // Chart instances
        this.charts = {};

        // Current domain being viewed
        this.currentDomain = "";

        // Initialization state
        this.initialized = false;
    }

    resetToDomainHistoryView() {
        if (this.tables.domainHistory) {

            const formatters = {
                hash: this.hashFormatter.bind(this),
                status: this.statusFormatter.bind(this),
                value: this.valueFormatter.bind(this),
                timestamp: this.timestampFormatter.bind(this),
                size: this.sizeFormatter.bind(this)
            };

            // Reset table columns for domain history
            this.tables.domainHistory.setColumns([{
                        title: "Transaction ID",
                        field: "verifiableID",
                        formatter: formatters.hash
                    }, {
                        title: "Type",
                        field: "type",
                        width: 100
                    }, {
                        title: "Value",
                        field: "value",
                        formatter: formatters.value,
                        width: 120
                    }, {
                        title: "From/To",
                        field: "counterparty",
                        formatter: formatters.hash
                    }, {
                        title: "Time",
                        field: "time",
                        formatter: formatters.timestamp,
                        width: 180
                    }
                ]);

            // Reset row click handler for domain history
            this.tables.domainHistory.off("rowClick");
            this.tables.domainHistory.on("rowClick", (e, row) => {
                if (this.callbacks?.onViewTransactionDetails) {
                    this.callbacks.onViewTransactionDetails(row.getData().verifiableID);
                }
            });
        }
    }

    collapseTransactionDetails(animate = true) {
        const detailsElement = this.mInstance.getControl("transaction-details");
        if (!detailsElement || !detailsElement.innerHTML.trim())
            return;

        if (animate) {
            // Remove any previous animation classes
            detailsElement.classList.remove('cyber-expanding');

            // Add digital noise and collapse animation
            detailsElement.classList.add('cyber-collapsing', 'cyber-glitch');

            // After animation completes, clear the content
            setTimeout(() => {
                detailsElement.innerHTML = "";
                detailsElement.classList.remove('cyber-collapsing', 'cyber-glitch');
            }, 800); // Match animation duration
        } else {
            detailsElement.innerHTML = "";
        }
    }

    collapseBlockDetails(animate = true) {
        const detailsElement = this.mInstance.getControl("block-details");
        if (!detailsElement || !detailsElement.innerHTML.trim())
            return;

        // Switch back to blocks list tab
        this.showBlocksListTab();

        if (animate) {
            // Remove any previous animation classes
            detailsElement.classList.remove('cyber-expanding');

            // Add digital noise and collapse animation
            detailsElement.classList.add('cyber-collapsing', 'cyber-glitch');

            // After animation completes, clear the content
            setTimeout(() => {
                detailsElement.innerHTML = "";
                detailsElement.classList.remove('cyber-collapsing', 'cyber-glitch');
            }, 800); // Match animation duration
        } else {
            detailsElement.innerHTML = "";
        }
    }

    /**
     * Initialize blocks section sub-tabs
     */
    initBlocksSubtabs() {
        const subtabBtns = $(this.mInstance.getBody).find('.blocks-subtabs .subtab-btn');
        const subtabContents = $(this.mInstance.getBody).find('.blocks-subtabs ~ .subtab-content');

        if (!subtabBtns.length) {
            console.warn('>>> [Explorer] Blocks subtabs not found in DOM');
            return;
        }

        subtabBtns.each((index, btn) => {
            btn.addEventListener('click', () => {
                const targetId = btn.getAttribute('data-subtab');
                this.switchBlocksSubtab(targetId);
            });
        });

        console.log('>>> [Explorer] Blocks subtabs initialized');
    }

    /**
     * Switch to a specific blocks sub-tab
     * @param {string} tabId - The ID of the tab to switch to ('blocks-list' or 'block-details-view')
     */
    switchBlocksSubtab(tabId) {
        const subtabBtns = $(this.mInstance.getBody).find('.blocks-subtabs .subtab-btn');
        const subtabContents = $(this.mInstance.getBody).find('#blocks-section .subtab-content');

        // Remove active class from all buttons and contents
        subtabBtns.each((index, btn) => {
            btn.classList.remove('active');
        });
        subtabContents.each((index, content) => {
            content.classList.remove('active');
        });

        // Add active class to target button and content
        const targetBtn = $(this.mInstance.getBody).find(`.subtab-btn[data-subtab="${tabId}"]`)[0];
        const targetContent = this.mInstance.getControl(tabId);

        if (targetBtn) {
            targetBtn.classList.add('active');
        }
        if (targetContent) {
            targetContent.classList.add('active');
        }

        // If switching to blocks list, redraw the table
        if (tabId === 'blocks-list' && this.tables.blocks) {
            setTimeout(() => {
                this.tables.blocks.redraw(true);
            }, 100);
        }

        console.log('>>> [Explorer] Switched to blocks subtab:', tabId);
    }

    /**
     * Switch to block details sub-tab (convenience method)
     */
    showBlockDetailsTab() {
        this.switchBlocksSubtab('block-details-view');
    }

    /**
     * Switch to blocks list sub-tab (convenience method)
     */
    showBlocksListTab() {
        this.switchBlocksSubtab('blocks-list');
    }

    formatLogEntries(logEntries) {
        if (!logEntries || !logEntries.length)
            return '';

        const formattedEntries = [];
        let executionBlock = false;

        logEntries.forEach((entry, index) => {
            // Skip empty entries
            if (!entry || entry.trim() === '')
                return;

            // Determine entry type based on content
            let entryType = 'info';

            if (entry.includes('Succeeded') ||
                entry.includes('True') ||
                entry.includes('✔') ||
                entry.includes('success')) {
                entryType = 'success';
            } else if (entry.includes('Failed') ||
                entry.includes('False') ||
                entry.includes('❌') ||
                entry.includes('error') ||
                entry.includes('Error')) {
                entryType = 'error';
            } else if (entry.includes('Warning') ||
                entry.includes('Caution') ||
                entry.includes('⚠')) {
                entryType = 'warning';
            }

            // Check if this is the start of an execution block
            if (entry.includes('Executing') ||
                (index > 0 && !executionBlock && entry.includes('sendEx'))) {
                executionBlock = true;
                formattedEntries.push('<div class="log-execution">');
            }

            // Add simulated timestamp for visual enhancement
            const timestamp = this.generateFakeTimestamp(index);

            // Format the entry with correct styling
            formattedEntries.push(
                `<div class="log-entry ${entryType}">` + 
`<span class="log-timestamp">[${timestamp}]</span>` +
                this.enhanceLogEntry(entry) +
`</div>`);
        });

        // Close execution block if opened
        if (executionBlock) {
            formattedEntries.push('</div>');
        }

        // Add a blinking cursor at the end for terminal effect
        formattedEntries.push('<span class="blink">_</span>');

        return formattedEntries.join('');
    }

    enhanceLogEntry(entry) {
        // Replace common patterns with enhanced formatting
        return entry
        .replace(/True/g, '<strong>True</strong>')
        .replace(/False/g, '<strong>False</strong>')
        .replace(/✔️/g, '<span style="color: #0cff0c;">✔️</span>')
        .replace(/❌/g, '<span style="color: #ff3860;">❌</span>')
        .replace(/\(([^)]+)\)/g, '<span style="color: #777;">($1)</span>')
        .replace(/(v:)/g, '<span style="color: #f4fb50;">$1</span>')
        .replace(/(sendEx|CSD|call|callEx)/g, '<span style="color: #b935f8;">$1</span>');
    }

    generateFakeTimestamp(index) {
        // Generate a plausible-looking timestamp for visual effect
        const date = new Date();
        date.setMilliseconds(date.getMilliseconds() + index * 17); // Slight offset per entry

        const hours = date.getHours().toString().padStart(2, '0');
        const minutes = date.getMinutes().toString().padStart(2, '0');
        const seconds = date.getSeconds().toString().padStart(2, '0');
        const millis = date.getMilliseconds().toString().padStart(3, '0');

        return `${hours}:${minutes}:${seconds}.${millis}`;
    }

    initLogDisplay(logValueId, toggleId, containerId) {
        // Use getControl or document.getElementById via the mInstance
        const toggle = this.mInstance.getControl(toggleId);
        const container = this.mInstance.getControl(containerId);

        if (!toggle || !container)
            return;

        // Add toggle functionality
        toggle.addEventListener('click', () => {
            if (container.style.display === 'none') {
                container.style.display = 'block';
                toggle.querySelector('.toggle-icon').textContent = '⬆';
            } else {
                container.style.display = 'none';
                toggle.querySelector('.toggle-icon').textContent = '⬇';
            }
        });

        // Add copy functionality
        const copyBtn = this.mInstance.getControl('copy-log-btn');
        const logValue = this.mInstance.getControl(logValueId);

        if (copyBtn && logValue) {
            copyBtn.addEventListener('click', () => {
                const logText = logValue.textContent;

                // Use navigator clipboard or create a temporary element for copying
                if (navigator.clipboard) {
                    navigator.clipboard.writeText(logText).then(() => {
                        this.showCopyNotification(container);
                    });
                } else {
                    // Fallback method
                    const textarea = document.createElement('textarea');
                    textarea.value = logText;
                    textarea.style.position = 'absolute';
                    textarea.style.left = '-9999px';
                    document.body.appendChild(textarea);
                    textarea.select();
                    document.execCommand('copy');
                    document.body.removeChild(textarea);
                    this.showCopyNotification(container);
                }
            });
        }
    }

    showCopyNotification(container) {
        // Create and show a "Copied!" notification
        const notification = document.createElement('div');
        notification.textContent = 'Copied!';
        notification.style.position = 'absolute';
        notification.style.right = '10px';
        notification.style.top = '10px';
        notification.style.background = 'rgba(12, 255, 12, 0.2)';
        notification.style.color = '#0cff0c';
        notification.style.padding = '5px 10px';
        notification.style.borderRadius = '3px';
        notification.style.fontSize = '12px';
        notification.style.transition = 'opacity 0.5s';

        container.appendChild(notification);

        setTimeout(() => {
            notification.style.opacity = '0';
            setTimeout(() => notification.remove(), 500);
        }, 1500);
    }

    escapeHtml(html) {
        const div = document.createElement('div');
        div.textContent = html;
        return div.innerHTML;
    }

    // Method to handle the source code highlighting and toggling
    initSourceCodeHighlighting(sourceCodeId, toggleId, containerId) {
        const toggle = this.mInstance.getControl(toggleId);
        const container = this.mInstance.getControl(containerId);
        const sourceCodeElement = this.mInstance.getControl(sourceCodeId);

        if (!toggle || !container || !sourceCodeElement)
            return;

        // Load highlight.js if not already loaded
        if (!window.hljs) {
            const script = document.createElement('script');
            script.src = 'https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.7.0/highlight.min.js';
            script.onload = () => {
                // Load GridScript definition
                const gridScriptScript = document.createElement('script');
                gridScriptScript.src = '/lib/gridscript-highlightjs-complete.js';
                gridScriptScript.onload = () => {
                    // Apply highlighting
                    window.hljs.highlightElement(sourceCodeElement);
                };
                document.head.appendChild(gridScriptScript);
            };
            document.head.appendChild(script);

            // Also load the highlight.js styles
            const style = document.createElement('link');
            style.rel = 'stylesheet';
            style.href = 'https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.7.0/styles/atom-one-dark.min.css';
            document.head.appendChild(style);
        } else {
            // If already loaded, apply highlighting directly
            window.hljs.highlightElement(sourceCodeElement);
        }

        // Add toggle functionality
        toggle.addEventListener('click', () => {
            if (container.style.display === 'none') {
                container.style.display = 'block';
                toggle.querySelector('.toggle-icon').textContent = '⬆';
            } else {
                container.style.display = 'none';
                toggle.querySelector('.toggle-icon').textContent = '⬇';
            }
        });

        // Add copy functionality
        const copyBtn = this.mInstance.getControl('copy-source-code-btn');
        if (copyBtn) {
            copyBtn.addEventListener('click', () => {
                const text = sourceCodeElement.textContent;
                navigator.clipboard.writeText(text).then(() => {
                    // Show a brief "Copied!" notification
                    const notification = document.createElement('div');
                    notification.textContent = 'Copied!';
                    notification.style.position = 'absolute';
                    notification.style.right = '10px';
                    notification.style.top = '10px';
                    notification.style.background = 'rgba(12, 255, 12, 0.2)';
                    notification.style.color = '#0cff0c';
                    notification.style.padding = '5px 10px';
                    notification.style.borderRadius = '3px';
                    notification.style.fontSize = '12px';
                    notification.style.transition = 'opacity 0.5s';

                    container.appendChild(notification);

                    setTimeout(() => {
                        notification.style.opacity = '0';
                        setTimeout(() => notification.remove(), 500);
                    }, 1500);
                });
            });
        }
    }

    /**
     * Refreshes status of all displayed transactions
     * Should be called periodically or when height changes.
     * Affects only visible items.
     */
    async refreshTableTransactionStatuses() {
        // Get current heights
        const vmContext = CVMContext.getInstance();
        const currentHeight = await vmContext.getHeightA(false);
        const currentKeyHeight = await vmContext.getHeightA(true);

        // Nothing to do if heights aren't available
        if (!currentHeight || !currentKeyHeight)
            return;

        // Update statuses for all transaction tables
        Object.values(this.tables).forEach(table => {
            if (!table)
                return;

            const data = table.getData();
            if (!Array.isArray(data) || !data.length)
                return;

            // Check if this table has transaction data
            const hasTx = data.some(row => row.verifiableID && row.result !== undefined);
            if (!hasTx)
                return;

            // Update with current heights and refresh rows
            let changed = false;
            const updatedData = data.map(row => {
                // Skip non-transactions
                if (!row.verifiableID || row.result === undefined)
                    return row;

                // We can't update the actual CTransactionDesc objects here
                // since we only have the plain objects, so we simulate the logic

                // Only update successful transactions at the appropriate point
                if (row.result !== 0 || !row.keyHeight)
                    return row;

                const difference = currentKeyHeight - row.keyHeight;
                const oldStatus = row.status;
                const newStatus = difference < 3 ? "Finalized (Pending)" : "Finalized (Safe)";

                if (oldStatus !== newStatus) {
                    changed = true;
                    return {
                        ...row,
                        status: newStatus
                    };
                }

                return row;
            });

            if (changed) {
                table.setData(updatedData);
            }
        });
    }
    /**
     * Check if a table is ready for operations
     * @param {string} tableId - The ID of the table to check
     * @returns {boolean} Whether the table is ready
     */
    isTableReady(tableId) {
        try {
            if (!this.tables || !this.tables[tableId]) {
                return false;
            }

            const table = this.tables[tableId];

            // Check if table has an element
            if (!table.element) {
                return false;
            }

            // Check if element is in the DOM
            if (!table.element.parentNode) {
                return false;
            }

            // Check if element is visible
            const style = window.getComputedStyle(table.element);
            if (style.display === 'none' || style.visibility === 'hidden') {
                return false;
            }

            // Check if table has been initialized
            if (table.element.getAttribute('data-tabulator-initialized') !== 'true') {
                return false;
            }

            return true;
        } catch (error) {
            console.warn(`Error checking table readiness for ${tableId}:`, error);
            return false;
        }
    }

    /**
     * Safely set table height
     * @param {string} tableId - The ID of the table
     * @param {number} height - The new height in pixels
     * @returns {boolean} Whether the operation was successful
     */
    safelySetTableHeight(tableId, height) {
        try {
            if (!this.isTableReady(tableId)) {
                return false;
            }

            const table = this.tables[tableId];
            table.setHeight(height);

            // Schedule a redraw for after DOM updates
            setTimeout(() => {
                try {
                    if (this.isTableReady(tableId)) {
                        this.tables[tableId].redraw(true);
                    }
                } catch (error) {
                    console.warn(`Error redrawing table ${tableId}:`, error);
                }
            }, 50);

            return true;
        } catch (error) {
            console.warn(`Error setting height for table ${tableId}:`, error);
            return false;
        }
    }

    /**
     * Safely redraw all visible tables
     */
    redrawVisibleTables() {
        try {
            const activeSection = this.mInstance.getActiveSection();

            // Map sections to their visible tables
            const sectionToTables = {
                dashboard: ['recentBlocks', 'recentTransactions'],
                blocks: ['blocks'],
                transactions: ['transactions'],
                domains: ['domainHistory']
            };

            // Get visible tables for current section
            const visibleTables = sectionToTables[activeSection] || [];

            // Redraw each visible table
            visibleTables.forEach(tableId => {
                if (this.isTableReady(tableId)) {
                    try {
                        this.tables[tableId].redraw(true);
                    } catch (error) {
                        console.warn(`Error redrawing table ${tableId}:`, error);
                    }
                }
            });
        } catch (error) {
            console.error("Error redrawing visible tables:", error);
        }
    }

    /**
     * Initialize UI components
     */
    initialize() {
        if (this.initialized)
            return;

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

        // Add market data button handler
        if (this.elements.marketDataButton) {
            this.elements.marketDataButton.addEventListener('click', () => {
                //const sortType = parseInt(this.elements.marketSortType.value);
                //const sortDir = parseInt(this.elements.marketSortOrder.value);
                this.callbacks.onGetMarketDomainData(1);
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

        // Initialize blocks section sub-tabs
        this.initBlocksSubtabs();

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
     * Sort BigInt values safely for Tabulator
     * @param {*} a First value
     * @param {*} b Second value
     * @param {Object} aRow First row data
     * @param {Object} bRow Second row data
     * @param {Object} column Column definition
     * @param {string} dir Sort direction ("asc" or "desc")
     * @returns {number} Standard numeric comparison result (-1, 0, 1)
     */
    bigIntSorter(a, b, aRow, bRow, column, dir) {
        // Handle null/undefined values

        if (a === b)
            return 0;
        if (a > b)
            return 1;
        else
            return -1;
    }
    /**
     * Initialize UI tables with Shadow DOM support and improved event handling
     */
    initTables() {
        // Create tables object if not exists
        this.tables = this.tables || {};
        // Create our custom BigInt sorter

        // Helper function to safely initialize a table
        const initTable = (tableId, config) => {
            try {
                const tableElement = this.mInstance.getControl(tableId);
                if (!tableElement) {
                    console.warn(`Table element ${tableId} not found in DOM`);
                    return null;
                }

                // Add custom attribute to help track tables in Shadow DOM
                tableElement.setAttribute('data-tabulator-initialized', 'false');

                // Enable sorting by default for all tables
                const finalConfig = {
                    ...config,
                    headerSort: true
                };

                // Create table with the provided config
                const table = new Tabulator(tableElement, finalConfig);

                // Mark as initialized for later checks
                tableElement.setAttribute('data-tabulator-initialized', 'true');

                return table;
            } catch (error) {
                console.error(`Error initializing table ${tableId}:`, error);
                return null;
            }
        };

        // Store formatter functions bound to this context to ensure proper operation
        const formatters = {
            hash: this.hashFormatter.bind(this),
            status: this.statusFormatter.bind(this),
            value: this.valueFormatter.bind(this),
            timestamp: this.timestampFormatter.bind(this),
            size: this.sizeFormatter.bind(this)
        };

        // Store sorter functions bound to this context
        const sorters = {
            bigint: this.bigIntSorter.bind(this)
        };

        // Initialize each table with proper configs
        this.tables.recentBlocks = initTable("recent-blocks-table", {
            height: "300px",
            layout: "fitColumns",
            placeholder: "No Block Data Available",
            columns: [{
                    title: "Height",
                    field: "height",
                    sorter: "number",
                    headerSortStartingDir: "desc",
                    width: 100
                }, {
                    title: "Block ID",
                    field: "blockID",
                    formatter: formatters.hash,
                    sorter: "string"
                }, {
                    title: "Miner",
                    field: "minerID",
                    formatter: formatters.hash,
                    sorter: "string"
                }, {
                    title: "Time",
                    field: "solvedAt",
                    formatter: formatters.timestamp,
                    sorter: "number",
                    width: 180
                }, {
                    title: "Transactions",
                    field: "transactionsCount",
                    sorter: "number",
                    width: 120
                }
            ]
        });

        // Add row click event handler
        if (this.tables.recentBlocks) {
            this.tables.recentBlocks.on("rowClick", (e, row) => {
                if (this.callbacks && this.callbacks.onViewBlockDetails) {
                    this.callbacks.onViewBlockDetails(row.getData().blockID);
                }
            });

            // Add cell hover effect
            this.tables.recentBlocks.on("cellMouseOver", (e, cell) => {
                const content = cell.getValue();
                if (content) {
                    cell.getElement().setAttribute("title", content);
                    cell.getElement().classList.add("hovered-cell");
                }
            });

            this.tables.recentBlocks.on("cellMouseOut", (e, cell) => {
                cell.getElement().classList.remove("hovered-cell");
            });
        }

        // Initialize other tables similarly
        this.tables.recentTransactions = initTable("recent-transactions-table", {
            height: "300px",
            layout: "fitColumns",
            placeholder: "No Transaction Data Available",
            columns: [{
                    title: "Transaction ID",
                    field: "verifiableID",
                    formatter: formatters.hash,
                    sorter: "string"
                }, {
                    title: "Status",
                    field: "status",
                    formatter: formatters.status,
                    width: 120,
                    sorter: (a, b, aRow, bRow) => {
                        // Add custom sorting for status
                        const aData = aRow.getData();
                        const bData = bRow.getData();
                        return (aData.result || 0) - (bData.result || 0);
                    }
                }, {
                    title: "From",
                    field: "sender",
                    formatter: formatters.hash,
                    sorter: sorters.bigint,
                }, {
                    title: "To",
                    field: "receiver",
                    formatter: formatters.hash,
                    sorter: sorters.bigint,
                }, {
                    title: "Value",
                    field: "value",
                    formatter: formatters.value,
                    sorter: sorters.bigint,
                    headerSort: true, // Explicitly enable header sorting
                    width: 120
                }, {
                    title: "Time",
                    field: "time",
                    formatter: formatters.timestamp,
                    sorter: sorters.bigint,
                    width: 180
                }
            ]
        });

        if (this.tables.recentTransactions) {
            this.tables.recentTransactions.on("rowClick", (e, row) => {
                if (this.callbacks && this.callbacks.onViewTransactionDetails) {
                    this.callbacks.onViewTransactionDetails(row.getData().verifiableID);
                }
            });

            this.tables.recentTransactions.on("cellMouseOver", (e, cell) => {
                const content = cell.getValue();
                if (content) {
                    cell.getElement().setAttribute("title", content);
                    cell.getElement().classList.add("hovered-cell");
                }
            });

            this.tables.recentTransactions.on("cellMouseOut", (e, cell) => {
                cell.getElement().classList.remove("hovered-cell");
            });
        }

        this.tables.blocks = initTable("blocks-table", {
            height: "600px",
            layout: "fitColumns",
            placeholder: "No Block Data Available",
            paginationButtonCount: 5,
            columns: [{
                    title: "Height",
                    field: "height",
                    sorter: "number",
                    headerSortStartingDir: "desc",
                    width: 100
                }, {
                    title: "Block ID",
                    field: "blockID",
                    formatter: formatters.hash,
                    sorter: "string"
                }, {
                    title: "Miner",
                    field: "minerID",
                    formatter: formatters.hash,
                    sorter: "string"
                }, {
                    title: "Time",
                    field: "solvedAt",
                    formatter: formatters.timestamp,
                    sorter: "number",
                    width: 180
                }, {
                    title: "Transactions",
                    field: "transactionsCount",
                    sorter: "number",
                    width: 120
                }, {
                    title: "Difficulty",
                    field: "difficulty",
                    sorter: "number",
                    width: 120
                }, {
                    title: "Size",
                    field: "size",
                    formatter: formatters.size,
                    sorter: "number",
                    width: 100
                }
            ]
        });

        if (this.tables.blocks) {
            this.tables.blocks.on("rowClick", (e, row) => {
                if (this.callbacks && this.callbacks.onViewBlockDetails) {
                    this.callbacks.onViewBlockDetails(row.getData().blockID);
                }
            });

            this.tables.blocks.on("cellMouseOver", (e, cell) => {
                const content = cell.getValue();
                if (content) {
                    cell.getElement().setAttribute("title", content);
                    cell.getElement().classList.add("hovered-cell");
                }
            });

            this.tables.blocks.on("cellMouseOut", (e, cell) => {
                cell.getElement().classList.remove("hovered-cell");
            });
        }

        this.tables.transactions = initTable("transactions-table", {
            height: "600px",
            layout: "fitColumns",
            placeholder: "No Transaction Data Available",
            paginationButtonCount: 5,
            columns: [{
                    title: "Transaction ID",
                    field: "verifiableID",
                    formatter: formatters.hash,
                    sorter: "string"
                }, {
                    title: "Status",
                    field: "status",
                    formatter: formatters.status,
                    sorter: "string",
                    width: 120
                }, {
                    title: "Block",
                    field: "height",
                    sorter: "number",
                    width: 100
                }, {
                    title: "From",
                    field: "sender",
                    formatter: formatters.hash,
                    sorter: "string"
                }, {
                    title: "To",
                    field: "receiver",
                    formatter: formatters.hash,
                    sorter: "string"
                }, {
                    title: "Value",
                    field: "value",
                    formatter: formatters.value,
                    sorter: "number",
                    width: 120
                }, {
                    title: "Time",
                    field: "time",
                    formatter: formatters.timestamp,
                    sorter: "number",
                    width: 180
                }
            ]
        });

        if (this.tables.transactions) {
            this.tables.transactions.on("rowClick", (e, row) => {
                if (this.callbacks && this.callbacks.onViewTransactionDetails) {
                    this.callbacks.onViewTransactionDetails(row.getData().verifiableID);
                }
            });

            this.tables.transactions.on("cellMouseOver", (e, cell) => {
                const content = cell.getValue();
                if (content) {
                    cell.getElement().setAttribute("title", content);
                    cell.getElement().classList.add("hovered-cell");
                }
            });

            this.tables.transactions.on("cellMouseOut", (e, cell) => {
                cell.getElement().classList.remove("hovered-cell");
            });
        }

        this.tables.domainHistory = initTable("domain-history-table", {
            height: "400px",
            layout: "fitColumns",
            selectable: true, // Explicitly enable row selection
            placeholder: "No Domain History Available",
            paginationButtonCount: 5,
            columns: [{
                    title: "Transaction ID",
                    field: "verifiableID",
                    formatter: formatters.hash,
                    sorter: "string"
                }, {
                    title: "Type",
                    field: "type",
                    sorter: "string",
                    width: 100
                }, {
                    title: "Value",
                    field: "value",
                    formatter: formatters.value,
                    sorter: "number",
                    width: 120
                }, {
                    title: "From/To",
                    field: "counterparty",
                    formatter: formatters.hash,
                    sorter: "string"
                }, {
                    title: "Time",
                    field: "time",
                    formatter: formatters.timestamp,
                    sorter: "number",
                    width: 180
                }
            ]
        });

        if (this.tables.domainHistory) {
            this.tables.domainHistory.on("rowClick", (e, row) => {
                if (this.callbacks && this.callbacks.onViewTransactionDetails) {
                    this.callbacks.onViewTransactionDetails(row.getData().verifiableID);
                }
            });

            this.tables.domainHistory.on("cellMouseOver", (e, cell) => {
                const content = cell.getValue();
                if (content) {
                    cell.getElement().setAttribute("title", content);
                    cell.getElement().classList.add("hovered-cell");
                }
            });

            this.tables.domainHistory.on("cellMouseOut", (e, cell) => {
                cell.getElement().classList.remove("hovered-cell");
            });
        }

        // Create external pagination controls for each table
        this.createPaginationControls();
    }

    /**
     * Create external pagination controls for each table
     */
    createPaginationControls() {
        // Define sections that need pagination
        const sections = ['blocks', 'transactions', 'domainHistory', 'search'];

        sections.forEach(section => {
            // Create or get existing pagination container
            let paginationContainer = this.elements.paginationControls[section];

            if (!paginationContainer) {
                // Get the parent element using window-scoped access
                let parentElement = this.mInstance.getControl(`${section}-section`);

                // For domainHistory, also try "domains-section" if not found
                if (!parentElement && section === 'domainHistory') {
                    parentElement = this.mInstance.getControl('domains-section');
                }

                if (!parentElement) {
                    console.warn(`Parent element for ${section} pagination not found`);
                    return;
                }

                // Determine the table element ID based on the section
                const tableId = section === 'domainHistory' ? 'domain-history-table' : `${section}-table`;

                // Get the table element using window-scoped access
                const tableEl = this.mInstance.getControl(tableId);
                if (!tableEl) {
                    console.warn(`Table element for ${section} pagination not found`);
                    return;
                }

                // Create a new pagination container after the table element
                // We create elements with document.createElement, but attach them to our window's elements
                paginationContainer = document.createElement('div');
                paginationContainer.id = `${section}-pagination`;
                paginationContainer.className = 'pagination-controls';
                tableEl.parentNode.insertBefore(paginationContainer, tableEl.nextSibling);

                // Save the new container in the elements store
                this.elements.paginationControls[section] = paginationContainer;
            }

            // Render pagination controls using the current pagination state for this section
            this.renderPaginationControls(section, this.paginationState[section]);
        });
    }

    /**
     * Render pagination controls for a specific section
     * @param {string} section - Section identifier (blocks, transactions, etc.)
     * @param {Object} state - Pagination state
     */
    renderPaginationControls(section, state) {
        const container = this.elements.paginationControls[section];
        if (!container)
            return;

        const {
            currentPage,
            pageSize,
            totalPages,
            totalItems
        } = state;

        // Create pagination controls HTML
        let html = `
    <div class="pagination-container">
      <div class="pagination-info">
        Page ${currentPage} of ${totalPages || 1} (${totalItems || 0} items)
      </div>
      <div class="pagination-buttons">
        <button class="pagination-button" data-page="first" ${currentPage <= 1 ? 'disabled' : ''}>
          <span>«</span>
        </button>
        <button class="pagination-button" data-page="prev" ${currentPage <= 1 ? 'disabled' : ''}>
          <span>‹</span>
        </button>
  `;

        // Add page number buttons
        const startPage = Math.max(1, currentPage - 2);
        const endPage = Math.min(totalPages || 1, startPage + 4);

        for (let i = startPage; i <= endPage; i++) {
            html += `
      <button class="pagination-button page-number ${i === currentPage ? 'active' : ''}" data-page="${i}">
        ${i}
      </button>
    `;
        }

        html += `
        <button class="pagination-button" data-page="next" ${currentPage >= (totalPages || 1) ? 'disabled' : ''}>
          <span>›</span>
        </button>
        <button class="pagination-button" data-page="last" ${currentPage >= (totalPages || 1) ? 'disabled' : ''}>
          <span>»</span>
        </button>
      </div>
      <div class="pagination-size">
        <label>Items per page:</label>
        <select class="page-size-select">
          <option value="10" ${pageSize === 10 ? 'selected' : ''}>10</option>
          <option value="20" ${pageSize === 20 ? 'selected' : ''}>20</option>
          <option value="50" ${pageSize === 50 ? 'selected' : ''}>50</option>
          <option value="100" ${pageSize === 100 ? 'selected' : ''}>100</option>
        </select>
      </div>
    </div>
  `;

        container.innerHTML = html;

        // Add event listeners
        this.addPaginationEventListeners(container, section);
    }

    /**
     * Add event listeners to pagination controls
     * @param {HTMLElement} container - Pagination container
     * @param {string} section - Section identifier
     */
    addPaginationEventListeners(container, section) {
        // Get pagination buttons
        const buttons = container.querySelectorAll('.pagination-button');

        buttons.forEach(button => {
            button.addEventListener('click', () => {
                if (button.hasAttribute('disabled'))
                    return;

                const pageAction = button.getAttribute('data-page');
                const currentPage = this.paginationState[section].currentPage;
                const totalPages = this.paginationState[section].totalPages || 1;

                let newPage = currentPage;

                // Determine the new page based on the button clicked
                switch (pageAction) {
                case 'first':
                    newPage = 1;
                    break;
                case 'prev':
                    newPage = Math.max(1, currentPage - 1);
                    break;
                case 'next':
                    newPage = Math.min(totalPages, currentPage + 1);
                    break;
                case 'last':
                    newPage = totalPages;
                    break;
                default:
                    // If it's a page number
                    if (!isNaN(pageAction)) {
                        newPage = parseInt(pageAction, 10);
                    }
                }

                if (newPage !== currentPage && this.callbacks && this.callbacks.onPageChange) {
                    this.callbacks.onPageChange(section, newPage);
                }
            });
        });

        // Page size select
        const pageSizeSelect = container.querySelector('.page-size-select');
        if (pageSizeSelect) {
            pageSizeSelect.addEventListener('change', () => {
                const newSize = parseInt(pageSizeSelect.value, 10);
                if (newSize !== this.paginationState[section].pageSize &&
                    this.callbacks && this.callbacks.onPageSizeChange) {
                    this.callbacks.onPageSizeChange(section, newSize);
                }
            });
        }
    }

    /**
     * Update pagination display for a section
     * @param {string} section - Section identifier
     * @param {Object} pagination - Pagination information
     */
    updatePagination(section, pagination) {
        // Update pagination state
        if (this.paginationState[section]) {
            Object.assign(this.paginationState[section], pagination);

            // Re-render pagination controls
            this.renderPaginationControls(section, this.paginationState[section]);
        }
    }

    /**
     * Register event callbacks
     * @param {Object} callbacks Object with callback functions
     */
    registerCallbacks(callbacks) {
        this.callbacks = callbacks;
        // Expose collapse methods for external use
        if (callbacks) {
            callbacks.collapseTransactionDetails = this.collapseTransactionDetails.bind(this);
            callbacks.collapseBlockDetails = this.collapseBlockDetails.bind(this);
        }
    }

    /**
     * Show or hide loading overlay
     * @param {boolean} show Whether to show loading
     */
    showLoading(show) {
        try {
            // Check if elements are initialized
            if (!this.elements) {
                console.warn("Elements not initialized, can't show/hide loading");
                return;
            }

            // Get loading overlay element
            const loadingOverlay = this.elements.loadingOverlay;
            if (!loadingOverlay) {
                console.warn("Loading overlay element not found");
                return;
            }

            // Show or hide loading overlay
            loadingOverlay.style.display = show ? 'flex' : 'none';
        } catch (error) {
            console.error("Error showing/hiding loading:", error);
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
     * Switch to a specific section with Shadow DOM support
     * @param {string} sectionId Section ID to switch to
     */
    switchToSection(sectionId, preventPanelClosing = false) {
        try {

            // Check if we're already in the target section
            const currentActiveNavItem =
                $(this.mInstance.getBody).find('.nav-item.active')[0];
            const alreadyInSection = currentActiveNavItem &&
                currentActiveNavItem.getAttribute('data-section') === sectionId;

            if (alreadyInSection) {
                this.mInstance.pauseCurtain();
                this.redrawVisibleTables();
                return;
            }

            if (!preventPanelClosing) {
                // Auto-collapse details when entering respective views
                if (sectionId === 'transactions') {
                    this.collapseTransactionDetails();
                } else if (sectionId === 'blocks') {
                    this.collapseBlockDetails();
                }
            }

            // Remove active class from all nav items and content sections
            if (this.elements.navItems) {
                this.elements.navItems.forEach(navItem => navItem.classList.remove('active'));
            }

            if (this.elements.contentSections) {
                this.elements.contentSections.forEach(section => section.classList.remove('active'));
            }

            // Add active class to specified nav item and content section
            const navItem =
                $(this.mInstance.getBody).find(`.nav-item[data-section="${sectionId}"]`)[0];

            if (navItem) {
                navItem.classList.add('active');
                const sectionElement = this.mInstance.getControl(`${sectionId}-section`);

                if (sectionElement) {
                    sectionElement.classList.add('active');

                    // Important: Tables in sections becoming visible need time to render
                    // before we try to resize/redraw them
                    setTimeout(() => {
                        // Calculate dimensions of tables in this section
                        const width = this.mInstance.getClientWidth || window.innerWidth;
                        const height = this.mInstance.getClientHeight || window.innerHeight;

                        // Use our helper methods to set table heights safely
                        switch (sectionId) {
                        case 'dashboard':
                            this.safelySetTableHeight('recentBlocks', height < 600 ? 200 : 300);
                            this.safelySetTableHeight('recentTransactions', height < 600 ? 200 : 300);
                            break;
                        case 'blocks':
                            this.safelySetTableHeight('blocks', height < 600 ? 400 : 600);
                            break;
                        case 'transactions':
                            this.safelySetTableHeight('transactions', height < 600 ? 400 : 600);
                            break;
                        case 'domains':
                            this.safelySetTableHeight('domainHistory', height < 600 ? 300 : 400);
                            break;
                        }

                        // Resize any charts in this section
                        if (sectionId === 'dashboard' && this.charts.transactionsChart) {
                            try {
                                Plotly.relayout(this.charts.transactionsChart, {
                                    width: Math.max(300, width - 40),
                                    height: 300,
                                    autosize: true
                                });
                            } catch (error) {
                                console.warn("Error resizing transactions chart:", error);
                            }
                        }

                        if (sectionId === 'statistics' && this.charts.dailyStatsChart) {
                            try {
                                Plotly.relayout(this.charts.dailyStatsChart, {
                                    width: Math.max(300, width - 40),
                                    height: 300,
                                    autosize: true
                                });
                            } catch (error) {
                                console.warn("Error resizing daily stats chart:", error);
                            }
                        }

                        // Redraw visible tables
                        this.redrawVisibleTables();

                    }, 100); // Delay to ensure DOM updates
                }
            }
        } catch (error) {
            console.error("Error switching sections:", error);
        }
    }

    /**
     * Update status display with latest values
     * @param {Object} data Status data
     */
    updateStatusDisplay(data) {
        if (!this.elements)
            return;

        // Update blockchain status
        if (data.blockchainStatus && this.elements.blockchainStatus) {
            const status = data.blockchainStatus.status || 'Live-Net';
            this.elements.blockchainStatus.textContent = `Network: ${status}`;
        }

        // Update height
        // minimalistic dial
        if (data.height !== undefined && this.elements.blockchainHeight) {
            this.elements.blockchainHeight.textContent = data.height.toString();
        }
        // dial within the animated header
        if (data.height !== undefined && this.elements.blockchainHeaderHeight) {
            this.elements.blockchainHeaderHeight.textContent = data.height.toString();
        }

        // Update USDT price
        if (data.usdtPrice !== undefined && this.elements.usdtPrice) {
            this.elements.usdtPrice.textContent = this.mTools.formatGNCValue(data.usdtPrice);
        }

        // Update Market Cap
        if (data.marketCap !== undefined && this.elements.marketCap) {
            this.elements.marketCap.textContent = this.mTools.formatGNCValue(data.marketCap);
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
        if (!this.elements || !data)
            return;

        try {

            // Update network utilization
            if (data.networkUtilization !== undefined && this.elements.networkUtilization) {
                const utilization = parseFloat(data.networkUtilization); // it's already a float but never enough for little bit of dedensive programming, hih?
                if (isNaN(utilization)) {
                    this.elements.networkUtilization.textContent = "N/A";
                } else {
                    // Format percentage to remove unnecessary trailing zeros
                    const formattedValue = utilization.toLocaleString(undefined, {
                        minimumFractionDigits: 0,
                        maximumFractionDigits: 2
                    });
                    this.elements.networkUtilization.textContent = `${formattedValue}%`;
                }
            }

            // Update average block size
            if (data.blockSize !== undefined && this.elements.avgBlockSize) {
                this.elements.avgBlockSize.textContent = this.mTools.formatByteSize(data.blockSize);
            }

            // Update block rewards
            if (data.blockRewards !== undefined && this.elements.blockRewards) {
                this.elements.blockRewards.textContent = this.mTools.formatGNCValue(data.blockRewards);
            }

            // Update average block time
            if (data.avgBlockTime !== undefined && this.elements.avgBlockTime) {
                const seconds = parseInt(data.avgBlockTime, 10);
                if (!isNaN(seconds)) {
                    this.elements.avgBlockTime.textContent =
                        this.mTools.secondsToFormattedString(seconds, true);
                } else {
                    this.elements.avgBlockTime.textContent = "N/A";
                }
            }
        } catch (error) {
            console.error("Error updating dashboard display:", error);
        }
    }

    /**
     * Update statistics display with latest values
     * @param {Object} data Statistics data
     */
    updateStatisticsDisplay(data) {
        if (!this.elements)
            return;

        // Update 24h stats
        if (data.avgKeyBlockTime !== undefined && this.elements.keyBlockTime) {
            this.elements.keyBlockTime.textContent = this.mTools.secondsToFormattedString(data.avgKeyBlockTime, true);
        }

        if (data.networkUtilization !== undefined && this.elements.networkUtil24h) {

            const utilization = parseFloat(data.networkUtilization); // it's already a float but never enough for little bit of dedensive programming, hih?
            if (isNaN(utilization)) {
                this.elements.networkUtil24h.textContent = "N/A";
            } else {
                // Format percentage to remove unnecessary trailing zeros
                const formattedValue = utilization.toLocaleString(undefined, {
                    minimumFractionDigits: 0,
                    maximumFractionDigits: 2
                });
                this.elements.networkUtil24h.textContent = `${formattedValue}%`;
            }

        }

        if (data.blockRewards !== undefined && this.elements.blockRewards24h) {
            this.elements.blockRewards24h.textContent = this.mTools.formatGNCValue(data.blockRewards);
        }

        if (data.blockSize !== undefined && this.elements.blockSize24h) {
            this.elements.blockSize24h.textContent = this.mTools.formatByteSize(data.blockSize);
        }
    }

    /**
     * Update all transaction statuses based on current blockchain height
     * @returns {Promise<void>}
     */
    async updateTransactionStatuses() {
        try {
            // Use the async getHeightA method to get current key height
            const currentKeyHeight = await CVMContext.getInstance().getHeightA(
                    true, // isKeyHeight = true
                    new ArrayBuffer(),
                    null,
                    eVMMetaCodeExecutionMode.RAW,
                    0,
                    30000, // timeoutMS
                    true // allowCache
                ) || 0;

            // Process each table sequentially
            for (const [tableId, table] of Object.entries(this.tables)) {
                if (!table || !tableId.toLowerCase().includes('transaction'))
                    continue;

                // Get current data
                const data = table.getData();
                if (!data || !data.length)
                    continue;

                // Check if we have transaction data
                const hasTransactions = data.some(row => row.verifiableID);
                if (!hasTransactions)
                    continue;

                // Update statuses
                let changed = false;
                const updatedData = data.map(row => {
                    // Skip non-transactions or already failed transactions
                    if (!row.verifiableID || !row.keyHeight || !row.confirmedTimestamp ||
                        (row.result !== 0 && row.result !== undefined)) {
                        return row;
                    }

                    // Check if status needs updating
                    const difference = currentKeyHeight - row.keyHeight;
                    const oldStatus = row.status;
                    const newStatus = difference < 3 ? "Finalized (Pending)" : "Finalized (Safe)";

                    if (oldStatus !== newStatus) {
                        changed = true;
                        return {
                            ...row,
                            status: newStatus
                        };
                    }

                    return row;
                });

                // Only update if statuses changed
                if (changed) {
                    table.setData(updatedData);
                }
            }
        } catch (error) {
            console.error("Error updating transaction statuses:", error);
        }
    }

    /**
     * Update transactions chart to display transaction activity over time
     * @param {Object} data Object containing transaction statistics with:
     *                      - dates: Array of Unix timestamps (seconds since epoch)
     *                      - counts: Array of transaction counts per day
     */
    updateTransactionsChart(data) {
        const chartContainer = this.mInstance.getControl('transactions-chart');
        if (!chartContainer || !data || data.length === 0)
            return;

        // Clear any existing chart to prevent stacking issues
        Plotly.purge(chartContainer);

        // Add CSS for pointer events to enable hover on title
        const styleId = 'transactions-chart-style';
        if (!document.getElementById(styleId)) {
            const styleEl = document.createElement('style');
            styleEl.id = styleId;
            styleEl.textContent = `
            .graphWrapperPointerEvents {
                pointer-events: all !important;
                cursor: help;
            }
            
            /* Essential fix for hover events while maintaining positioning */
            .js-plotly-plot .plotly {
                position: relative !important;
            }
            
            /* Ensure proper z-index for SVG layers */
            .js-plotly-plot .plotly .main-svg:nth-child(1) {
                position: absolute !important;
                z-index: 1 !important;
                pointer-events: auto !important;
            }
            
            .js-plotly-plot .plotly .main-svg:nth-child(2) {
                position: absolute !important;
                z-index: 10 !important;
                pointer-events: none !important;
            }
            
            .js-plotly-plot .plotly .main-svg:nth-child(3) {
                position: absolute !important;
                z-index: 100 !important;
                pointer-events: none !important;
            }
            
            /* Fix for hover text */
            .js-plotly-plot .plotly .hoverlayer {
                z-index: 1000 !important;
            }
            
            /* Make sure hover text is visible */
            .js-plotly-plot .plotly .hovertext {
                fill: white !important;
            }
            
            /* Ensure hover boxes appear correctly */
            .js-plotly-plot .plotly .hoverbox {
                fill: rgba(0,0,0,0.8) !important;
            }
            
            /* Enable pointer events on plot elements */
            .js-plotly-plot .plotly .scatterlayer .trace {
                pointer-events: auto !important;
            }
            
            /* Allow hovering on data points */
            .js-plotly-plot .plotly .points path {
                pointer-events: auto !important;
            }
        `;
            document.head.appendChild(styleEl);
        }

        // Ensure we have dates and counts, whether from old or new data format
        let dates = [];
        let counts = [];
        let volumes = null;

        // Handle both array of objects (old format) and object with arrays (new format)
        if (Array.isArray(data)) {
            // Old format - array of objects
            dates = data.map(stat => new Date(stat.date * 1000).toLocaleDateString());
            counts = data.map(stat => stat.count || 0);
            volumes = data.map(stat => stat.volume || 0);
        } else if (data.dates && data.counts) {
            // New format - object with date and count arrays
            dates = data.dates.map(timestamp => new Date(timestamp * 1000).toLocaleDateString());
            counts = data.counts;
            if (data.volumes && data.volumes.length === data.dates.length) {
                volumes = data.volumes;
            }
        } else {
            console.error("Invalid data format for transactions chart");
            return;
        }

        if (dates.length === 0 || counts.length === 0) {
            console.error("No valid data for transactions chart");
            return;
        }

        // Reverse datasets to correct data flow on x-axis
        dates.reverse();
        counts.reverse();
        if (volumes)
            volumes.reverse();

        // Create chart data array - start with transactions count
        const chartData = [{
                x: dates,
                y: counts,
                type: 'scatter',
                mode: 'lines+markers',
                name: 'Transaction Count',
                marker: {
                    color: '#b935f8',
                    size: 8
                },
                line: {
                    color: '#b935f8',
                    width: 2
                },
                // Enhanced hover template with explicit formatting
                hovertemplate: '<b>Date:</b> %{x}<br>' +
                '<b>Transactions:</b> %{y}<br>' +
                '<extra></extra>'
            }
        ];

        // Create layout with centered title
        const layout = {
            title: {
                text: 'GRIDNET Daily Transactions',
                font: {
                    color: '#ffffff',
                    size: 20
                },
                x: 0.5, // Center the title
                xanchor: 'center'
            },
            paper_bgcolor: 'rgba(0,0,0,0)',
            plot_bgcolor: 'rgba(0,0,0,0)',
            font: {
                color: '#ffffff'
            },
            xaxis: {
                title: {
                    text: 'Date',
                    font: {
                        color: '#ffffff',
                        size: 14
                    }
                },
                gridcolor: 'rgba(61, 110, 255, 0.1)'
            },
            yaxis: {
                title: {
                    text: 'Number of Transactions',
                    font: {
                        color: '#b935f8',
                        size: 14
                    }
                },
                tickfont: {
                    color: '#b935f8'
                },
                gridcolor: 'rgba(61, 110, 255, 0.1)'
            },
            legend: {
                orientation: 'h',
                y: -0.2,
                font: {
                    color: '#ffffff'
                }
            },
            margin: {
                l: 80,
                r: 60,
                t: 60, // Increased to make room for title
                b: 80
            },
            width: chartContainer.offsetWidth || chartContainer.clientWidth,
            height: 300,
            hovermode: 'closest', // Enable precise hover
            // Enhanced hover configuration
            hoverlabel: {
                bgcolor: 'rgba(0,0,0,0.8)',
                font: {
                    color: '#ffffff',
                    size: 12
                },
                bordercolor: '#b935f8'
            }
        };

        // Add volume data and axis only if we have volume data
        if (volumes && volumes.length === dates.length && volumes.some(v => v > 0)) {
            // Add volume data series with enhanced hover template
            chartData.push({
                x: dates,
                y: volumes,
                type: 'scatter',
                mode: 'lines+markers',
                name: 'Volume (GNC)',
                yaxis: 'y2',
                marker: {
                    color: '#0cffff',
                    size: 8
                },
                line: {
                    color: '#0cffff',
                    width: 2
                },
                hovertemplate: '<b>Date:</b> %{x}<br>' +
                '<b>Volume:</b> %{y} GNC<br>' +
                '<extra></extra>'
            });

            // Add second y-axis to layout
            layout.yaxis2 = {
                title: {
                    text: 'Transaction Volume (GNC)',
                    font: {
                        color: '#0cffff',
                        size: 14
                    }
                },
                tickfont: {
                    color: '#0cffff'
                },
                overlaying: 'y',
                side: 'right',
                gridcolor: 'rgba(0,0,0,0)'
            };
        }

        const config = {
            responsive: true,
            displayModeBar: false,
            showTips: true // Show tooltip info
        };

        try {
            // Create the plot
            Plotly.newPlot(chartContainer, chartData, layout, config);
            this.charts.transactionsChart = chartContainer;

            // Add long description tooltip to the title and fix positioning/hover issues
            setTimeout(() => {
                try {
                    // 1. Fix title element tooltip
                    const titleElement = chartContainer.querySelector('g.g-gtitle > text.gtitle');
                    if (titleElement) {
                        titleElement.setAttribute('title', 'This chart shows the daily transaction count on the GRIDNET blockchain. Each point represents the total number of transactions processed on that date.');
                        titleElement.classList.add('graphWrapperPointerEvents');

                        // Use tippy if available or fallback to default browser tooltip
                        if (typeof tippy === 'function') {
                            tippy(titleElement, {
                                content: 'This chart shows the daily transaction count on the GRIDNET blockchain. Each point represents the total number of transactions processed on that date.',
                                theme: 'dark',
                                placement: 'bottom',
                                zIndex: 10000 // Ensure it appears above everything
                            });
                        }
                    }

                    // 2. Fix positioning of all SVG elements for proper layering
                    const svgContainer = chartContainer.querySelector('.svg-container');
                    if (svgContainer) {
                        svgContainer.style.position = 'relative';
                    }

                    // 3. Fix z-index for SVG layers to ensure proper stacking
                    const svgElements = chartContainer.querySelectorAll('.main-svg');
                    svgElements.forEach((svg, index) => {
                        svg.style.position = 'absolute';
                        svg.style.top = '0';
                        svg.style.left = '0';

                        // Set z-index based on layer order
                        if (index === 0) {
                            // Base layer - needs to receive events
                            svg.style.zIndex = '1';
                            svg.style.pointerEvents = 'auto';
                        } else if (index === svgElements.length - 1) {
                            // Top layer (usually for hover effects)
                            svg.style.zIndex = '100';
                            svg.style.pointerEvents = 'none';
                        } else {
                            // Middle layers
                            svg.style.zIndex = '10';
                            svg.style.pointerEvents = 'none';
                        }
                    });

                    // 4. Ensure hover layer properly overlays chart
                    const hoverLayer = chartContainer.querySelector('.hoverlayer');
                    if (hoverLayer) {
                        hoverLayer.style.zIndex = '1000';
                        // Crucial: Allow pointer events through to underlying elements
                        hoverLayer.style.pointerEvents = 'none';
                    }

                    // 5. Ensure container has position relative to anchor absolute SVGs
                    chartContainer.style.position = 'relative';

                    // 6. Make scatter layer interactive but keep hover working
                    const scatterLayers = chartContainer.querySelectorAll('.scatterlayer .trace');
                    scatterLayers.forEach(layer => {
                        layer.style.pointerEvents = 'auto';
                    });

                    // 7. Make sure points are hoverable
                    const points = chartContainer.querySelectorAll('.points path');
                    points.forEach(point => {
                        point.style.pointerEvents = 'auto';
                    });

                    // 8. Fix hover text visibility
                    const hoverTexts = chartContainer.querySelectorAll('.hovertext');
                    hoverTexts.forEach(text => {
                        text.style.fill = 'white';
                    });

                } catch (error) {
                    console.warn("Could not apply positioning and hover fixes:", error);
                }
            }, 100);
        } catch (error) {
            console.error("Error creating transactions chart:", error);
        }
    }

    /**
     * Update daily stats chart with comprehensive blockchain metrics
     * @param {Array} data Array of flattened CTransactionDesc objects
     */
    updateDailyStatsChart(data) {
        const chartContainer = this.mInstance.getControl('daily-stats-chart');
        if (!chartContainer || !data || data.length === 0)
            return;

        // Clear any existing chart to prevent stacking issues
        Plotly.purge(chartContainer);

        // Add CSS for pointer events to enable hover on title
        const styleId = 'daily-stats-chart-style';
        if (!document.getElementById(styleId)) {
            const styleEl = document.createElement('style');
            styleEl.id = styleId;
            styleEl.textContent = `
        .graphWrapperPointerEvents {
            pointer-events: all !important;
            cursor: help;
        }
        
        /* Essential fix for hover events while maintaining positioning */
        .js-plotly-plot .plotly {
            position: relative !important;
        }
        
        /* Ensure proper z-index for SVG layers */
        .js-plotly-plot .plotly .main-svg:nth-child(1) {
            position: absolute !important;
            z-index: 1 !important;
            pointer-events: auto !important;
        }
        
        .js-plotly-plot .plotly .main-svg:nth-child(2) {
            position: absolute !important;
            z-index: 10 !important;
            pointer-events: none !important;
        }
        
        .js-plotly-plot .plotly .main-svg:nth-child(3) {
            position: absolute !important;
            z-index: 100 !important;
            pointer-events: none !important;
        }
        
        /* Fix for hover text */
        .js-plotly-plot .plotly .hoverlayer {
            z-index: 1000 !important;
        }
        
        /* Make sure hover text is visible */
        .js-plotly-plot .plotly .hovertext {
            fill: white !important;
        }
        
        /* Ensure hover boxes appear correctly */
        .js-plotly-plot .plotly .hoverbox {
            fill: rgba(0,0,0,0.8) !important;
        }
        
        /* Enable pointer events on plot elements */
        .js-plotly-plot .plotly .scatterlayer .trace {
            pointer-events: auto !important;
        }
        
        /* Allow hovering on data points */
        .js-plotly-plot .plotly .points path {
            pointer-events: auto !important;
        }
        
        /* Fix SVG container width */
        .js-plotly-plot .svg-container {
            width: 100% !important;
            max-width: 100% !important;
        }
        
        /* Fix SVG elements width */
        .js-plotly-plot .main-svg {
            width: 100% !important;
            max-width: 100% !important;
        }
    `;
            document.head.appendChild(styleEl);
        }

        // Group transactions by date
        const dailyStats = this.aggregateTransactionsByDate(data);

        // Extract chart data from aggregated stats
        const dates = Object.keys(dailyStats).sort();
        const formattedDates = dates.map(dateStr => new Date(dateStr).toLocaleDateString());

        // Extract metrics for the chart
        const transactionCounts = dates.map(date => dailyStats[date].count);
        const totalValues = dates.map(date => dailyStats[date].totalValue);
        const totalFees = dates.map(date => dailyStats[date].totalFee);

        // Create gradient fills for the graphs
        const volumeGradient = [{
                offset: 0,
                color: 'rgba(61, 110, 255, 0.7)'
            }, {
                offset: 1,
                color: 'rgba(61, 110, 255, 0.1)'
            }
        ];

        const feesGradient = [{
                offset: 0,
                color: 'rgba(12, 255, 12, 0.7)'
            }, {
                offset: 1,
                color: 'rgba(12, 255, 12, 0.1)'
            }
        ];

        const countGradient = [{
                offset: 0,
                color: 'rgba(185, 53, 248, 0.7)'
            }, {
                offset: 1,
                color: 'rgba(185, 53, 248, 0.1)'
            }
        ];

        // Create chart data with enhanced aesthetics
        const chartData = [{
                x: formattedDates,
                y: totalValues,
                type: 'scatter',
                mode: 'lines+markers',
                name: 'Volume (GNC)',
                yaxis: 'y',
                marker: {
                    color: '#3d6eff',
                    size: 8,
                    line: {
                        color: '#ffffff',
                        width: 1
                    }
                },
                line: {
                    color: '#3d6eff',
                    width: 3,
                    shape: 'spline'
                },
                fill: 'tozeroy',
                fillcolor: {
                    type: 'gradient',
                    colorscale: volumeGradient
                },
                // Enhanced hover template
                hovertemplate: 'Date: %{x}<br>Volume: %{y} GNC<extra></extra>'
            }, {
                x: formattedDates,
                y: totalFees,
                type: 'bar',
                name: 'Fees (nGNC)',
                yaxis: 'y2',
                marker: {
                    color: '#0cff0c',
                    opacity: 0.8,
                    line: {
                        color: '#ffffff',
                        width: 1
                    }
                },
                // Enhanced hover template
                hovertemplate: 'Date: %{x}<br>Fees: %{y} nGNC<extra></extra>'
            }, {
                x: formattedDates,
                y: transactionCounts,
                type: 'scatter',
                mode: 'lines+markers',
                name: 'Transaction Count',
                yaxis: 'y3',
                marker: {
                    color: '#b935f8',
                    size: 8,
                    symbol: 'diamond',
                    line: {
                        color: '#ffffff',
                        width: 1
                    }
                },
                line: {
                    color: '#b935f8',
                    width: 3,
                    dash: 'solid',
                    shape: 'spline'
                },
                fill: 'tozeroy',
                fillcolor: {
                    type: 'gradient',
                    colorscale: countGradient
                },
                // Enhanced hover template
                hovertemplate: 'Date: %{x}<br>Transactions: %{y}<extra></extra>'
            }
        ];

        const layout = {
            title: {
                text: 'ACTIVITY METRICS',
                font: {
                    family: 'Consolas, Arial, sans-serif',
                    size: 16,
                    color: 'rgb(158 247 233)'
                },
                x: 0.5, // Center the title
                xanchor: 'center'
            },
            paper_bgcolor: 'rgba(10, 11, 22, 0.9)',
            plot_bgcolor: 'rgba(5, 7, 20, 0.5)',
            font: {
                family: 'Consolas, Monaco, monospace',
                color: '#ffffff'
            },
            // Remove grid setup and use specific subplot arrangement
            // with domains instead
            xaxis: {
                title: {
                    text: 'DATE',
                    font: {
                        family: 'Orbitron, Arial, sans-serif',
                        size: 14
                    }
                },
                gridcolor: 'rgba(61, 110, 255, 0.2)',
                zeroline: false,
                showgrid: true,
                domain: [0, 0.95], // Give space for y-axis
                // Add auto-scaling and range padding for better fit
                autorange: true,
                rangemode: 'normal',
                // Ensure dates display properly
                type: 'category'
            },
            yaxis: {
                // Remove or simplify title since we have the legend
                title: {
                    text: '', // Removed title
                    font: {
                        family: 'Orbitron, Arial, sans-serif',
                        size: 14,
                        color: '#3d6eff'
                    }
                },
                tickfont: {
                    color: '#3d6eff'
                },
                gridcolor: 'rgba(61, 110, 255, 0.2)',
                zeroline: false,
                domain: [0.7, 1]// Top third of chart area
            },
            yaxis2: {
                // Remove or simplify title since we have the legend
                title: {
                    text: '', // Removed title
                    font: {
                        family: 'Orbitron, Arial, sans-serif',
                        size: 14,
                        color: '#0cff0c'
                    }
                },
                tickfont: {
                    color: '#0cff0c'
                },
                gridcolor: 'rgba(12, 255, 12, 0.2)',
                zeroline: false,
                domain: [0.35, 0.65], // Middle third of chart area
                anchor: 'x'
            },
            yaxis3: {
                // Remove or simplify title since we have the legend
                title: {
                    text: '', // Removed title
                    font: {
                        family: 'Orbitron, Arial, sans-serif',
                        size: 14,
                        color: '#b935f8'
                    }
                },
                tickfont: {
                    color: '#b935f8'
                },
                gridcolor: 'rgba(185, 53, 248, 0.2)',
                zeroline: false,
                domain: [0, 0.3], // Bottom third of chart area
                anchor: 'x'
            },
            legend: {
                orientation: 'h',
                y: -0.2,
                x: 0.5, // Center the legend
                xanchor: 'center',
                bgcolor: 'rgba(10, 11, 22, 0.7)',
                bordercolor: 'rgba(59, 110, 255, 0.5)',
                borderwidth: 1
            },
            margin: {
                l: 50, // Reduced left margin since we removed y-axis titles
                r: 20, // Reduced right margin to prevent overflow
                t: 60, // Top margin for title
                b: 100 // Increased bottom margin for legend
            },
            autosize: true, // Enable autosize
            responsive: true, // Make the chart responsive,
            annotations: [{
                    xref: 'paper',
                    yref: 'paper',
                    x: 1,
                    y: -0.3,
                    text: 'GRIDNET BLOCKCHAIN STATISTICS',
                    showarrow: false,
                    font: {
                        family: 'Orbitron, Arial, sans-serif',
                        size: 12,
                        color: 'rgba(255, 255, 255, 0.5)'
                    }
                }
            ],
            // Enhanced hover configuration
            hoverlabel: {
                bgcolor: 'rgba(0,0,0,0.8)',
                font: {
                    color: '#ffffff',
                    size: 12
                }
            },
            hovermode: 'closest' // Enable precise hover
        };

        const config = {
            responsive: true,
            displayModeBar: false,
            scrollZoom: false,
            showTips: true, // Show tooltip info
            // Limit the container size
            autosizable: true,
            fillFrame: false
        };

        try {
            // Calculate proper width based on container
            const containerWidth = chartContainer.offsetWidth || chartContainer.clientWidth;
            // Set chart width to be slightly less than container to prevent overflow
            const chartWidth = Math.max(Math.min(containerWidth - 40, containerWidth * 0.95), 300);

            // Update layout with calculated width
            layout.width = chartWidth;
            layout.autosize = false; // Prevent Plotly from auto-sizing

            Plotly.newPlot(chartContainer, chartData, layout, config);

            // Fix SVG container width after plot is created
            setTimeout(() => {
                try {
                    // Fix the SVG container width to match the chart container
                    const svgContainer = chartContainer.querySelector('.svg-container');
                    if (svgContainer) {
                        svgContainer.style.width = '100%';
                        svgContainer.style.maxWidth = '100%';
                    }

                    // Fix SVG elements width
                    const svgElements = chartContainer.querySelectorAll('.main-svg');
                    svgElements.forEach(svg => {
                        svg.setAttribute('width', '100%');
                        svg.style.width = '100%';
                        svg.style.maxWidth = '100%';
                    });

                    // Add neon glow effect to the chart container
                    chartContainer.style.boxShadow = '0 0 15px rgba(61, 110, 255, 0.5)';
                    chartContainer.style.borderRadius = '8px';
                    chartContainer.style.border = '1px solid rgba(61, 110, 255, 0.3)';

                    // Ensure container is position relative
                    chartContainer.style.position = 'relative';
                    chartContainer.style.overflow = 'hidden'; // Prevent any overflow

                    // 1. Fix title element tooltip
                    const titleElement = chartContainer.querySelector('g.g-gtitle > text.gtitle');
                    if (titleElement) {
                        titleElement.setAttribute('title', 'This chart shows daily blockchain activity metrics including transaction volume, fees, and count.');
                        titleElement.classList.add('graphWrapperPointerEvents');

                        // Use tippy if available or fallback to default browser tooltip
                        if (typeof tippy === 'function') {
                            tippy(titleElement, {
                                content: 'This chart shows daily blockchain activity metrics including transaction volume, fees, and count.',
                                theme: 'dark',
                                placement: 'bottom',
                                zIndex: 10000 // Ensure it appears above everything
                            });
                        }
                    }

                    // 2. Fix positioning of all SVG elements for proper layering
                    if (svgContainer) {
                        svgContainer.style.position = 'relative';
                    }

                    // 3. Fix z-index for SVG layers to ensure proper stacking
                    svgElements.forEach((svg, index) => {
                        svg.style.position = 'absolute';
                        svg.style.top = '0';
                        svg.style.left = '0';

                        // Set z-index based on layer order
                        if (index === 0) {
                            // Base layer - needs to receive events
                            svg.style.zIndex = '1';
                            svg.style.pointerEvents = 'auto';
                        } else if (index === svgElements.length - 1) {
                            // Top layer (usually for hover effects)
                            svg.style.zIndex = '100';
                            svg.style.pointerEvents = 'none';
                        } else {
                            // Middle layers
                            svg.style.zIndex = '10';
                            svg.style.pointerEvents = 'none';
                        }
                    });

                    // 4. Ensure hover layer properly overlays chart
                    const hoverLayer = chartContainer.querySelector('.hoverlayer');
                    if (hoverLayer) {
                        hoverLayer.style.zIndex = '1000';
                        // Crucial: Allow pointer events through to underlying elements
                        hoverLayer.style.pointerEvents = 'none';
                    }

                    // 5. Make scatter layer interactive but keep hover working
                    const scatterLayers = chartContainer.querySelectorAll('.scatterlayer .trace');
                    scatterLayers.forEach(layer => {
                        layer.style.pointerEvents = 'auto';
                    });

                    // 6. Make sure points are hoverable
                    const points = chartContainer.querySelectorAll('.points path');
                    points.forEach(point => {
                        point.style.pointerEvents = 'auto';
                    });

                    // 7. Fix hover text visibility
                    const hoverTexts = chartContainer.querySelectorAll('.hovertext');
                    hoverTexts.forEach(text => {
                        text.style.fill = 'white';
                    });

                    // 8. Fix legend positioning and interaction
                    const legendItems = chartContainer.querySelectorAll('.legend .traces');
                    if (legendItems.length > 0) {
                        const legendContainer = chartContainer.querySelector('.legend');
                        if (legendContainer) {
                            legendContainer.style.pointerEvents = 'auto';
                        }

                        legendItems.forEach(item => {
                            item.style.pointerEvents = 'auto';
                            item.style.cursor = 'pointer';
                        });
                    }
                } catch (error) {
                    console.warn("Could not apply positioning and hover fixes:", error);
                }
            }, 50);

            this.charts.dailyStatsChart = chartContainer;

            // Add window resize handler to adjust chart size
            const resizeChart = () => {
                if (!chartContainer || !this.charts.dailyStatsChart)
                    return;

                try {
                    // Get new container width
                    const newWidth = chartContainer.offsetWidth || chartContainer.clientWidth;
                    const adjustedWidth = Math.max(Math.min(newWidth - 40, newWidth * 0.95), 300);

                    Plotly.relayout(chartContainer, {
                        width: adjustedWidth
                    });

                    // Fix SVG container width after relayout
                    setTimeout(() => {
                        const svgContainer = chartContainer.querySelector('.svg-container');
                        if (svgContainer) {
                            svgContainer.style.width = '100%';
                            svgContainer.style.maxWidth = '100%';
                        }

                        const svgElements = chartContainer.querySelectorAll('.main-svg');
                        svgElements.forEach(svg => {
                            svg.setAttribute('width', '100%');
                            svg.style.width = '100%';
                            svg.style.maxWidth = '100%';
                        });
                    }, 50);
                } catch (error) {
                    console.warn("Error resizing chart:", error);
                }
            };

            // Add resize event listener
            const resizeObserver = new ResizeObserver(resizeChart);
            resizeObserver.observe(chartContainer);

            // Store observer for cleanup
            if (!this.resizeObservers)
                this.resizeObservers = {};
            // Clean up any existing observer for this chart
            if (this.resizeObservers.dailyStatsChart) {
                this.resizeObservers.dailyStatsChart.disconnect();
            }
            this.resizeObservers.dailyStatsChart = resizeObserver;

        } catch (error) {
            console.error("Error creating daily stats chart:", error);
        }
    }
    /**
     * Helper method to aggregate transaction data by date with proper GNC units
     * @param {Array} transactions Array of flattened transaction objects
     * @returns {Object} Daily stats aggregated by date
     */
    aggregateTransactionsByDate(transactions) {
        const dailyStats = {};

        transactions.forEach(tx => {
            // Get timestamp from confirmed or time field, defaulting to current time
            const timestamp = tx.confirmedTimestamp || tx.time || Math.floor(Date.now() / 1000);
            const date = new Date(timestamp * 1000);
            const dateStr = date.toISOString().split('T')[0]; // YYYY-MM-DD

            // Initialize stats object for this date if it doesn't exist
            if (!dailyStats[dateStr]) {
                dailyStats[dateStr] = {
                    count: 0,
                    totalValue: 0,
                    totalFee: 0,
                    totalERGUsed: 0,
                    avgERGUsed: 0,
                    typeDistribution: {}
                };
            }

            // Update stats - Convert from atto to proper GNC units
            const stats = dailyStats[dateStr];
            stats.count++;
            // Convert to BigInt and then divide by 10^18 to get GNC units
            const valueInGNC = parseFloat(tx.value || 0) / 1e18;
            const feeInNanoGNC = parseFloat(tx.fee || 0) / 1e9;

            stats.totalValue += valueInGNC;
            stats.totalFee += feeInNanoGNC;
            stats.totalERGUsed += parseFloat(tx.ERGUsed || 0);
            stats.avgERGUsed = stats.totalERGUsed / stats.count;

            // Track transaction type distribution
            const txType = tx.type || 'unknown';
            stats.typeDistribution[txType] = (stats.typeDistribution[txType] || 0) + 1;
        });

        return dailyStats;
    }

    
    displayBlockDetails(block) {
        const blockDetailsElement = this.mInstance.getControl('block-details');
        if (!blockDetailsElement)
            return;

        if (!block) {
            blockDetailsElement.innerHTML = '<p>No block details available</p>';
            return;
        }

        // Auto-switch to block details tab
        this.showBlockDetailsTab();

        // First clear any existing content (no animation if replacing)
        if (blockDetailsElement.innerHTML.trim()) {
            blockDetailsElement.innerHTML = "";
        }

        const isKeyBlock = block.type === eBlockType.keyBlock;

        // Format block details for display
        const html = `
    <div class="detail-view fade-in">
      <div class="detail-header">
        <div class="detail-type">Block Details</div>
        <div style="display: flex; align-items: center; gap: 10px;">
          <span>Height: ${block.height}</span>
          <button class="glink-copy-btn" id="copy-block-glink-btn" title="Share this block">
            <i class="fa fa-share-alt"></i> Share
          </button>
        </div>
      </div>
      
      <div class="detail-item">
        <div class="detail-label">Block ID</div>
        <div class="detail-value hash-value" id="block-id-value">${block.blockID}</div>
      </div>
	  
	   <div class="detail-item">
        <div class="detail-label">Size</div>
        <div class="detail-value"> ${this.mTools.formatByteSize(block.size && block.size || 0)}</div>
      </div>
      
      <div class="detail-item">
        <div class="detail-label">Timestamp</div>
        <div class="detail-value">${new Date(block.solvedAt * 1000).toLocaleString()}</div>
      </div>
      
      <div class="detail-item">
        <div class="detail-label">Miner</div>
        <div class="detail-value hash-value" id="miner-id-value">${block.minerID}</div>
      </div>
      
      <div class="detail-item">
        <div class="detail-label">Parent Block</div>
        <div class="detail-value hash-value" id="parent-id-value">${block.parentID}</div>
      </div>
      
      <div class="detail-item">
        <div class="detail-label">Key Height</div>
        <div class="detail-value">${block.keyHeight}</div>
      </div>
      
      <div class="detail-item">
        <div class="detail-label">Type</div>
        <div class="detail-value">${isKeyBlock ? 'Key Block' : 'Data Block'}</div>
      </div>
      
      ${isKeyBlock ? `
      <div class="detail-item">
        <div class="detail-label">Difficulty</div>
        <div class="detail-value" id="difficulty-value">${block.difficulty}</div>
      </div>
      
      <div class="detail-item">
        <div class="detail-label">Total Difficulty</div>
        <div class="detail-value" id="total-difficulty-value">${block.totalDifficulty}</div>
      </div>
      ` : ''}
      
      <div class="detail-item">
        <div class="detail-label">Nonce</div>
        <div class="detail-value" id="nonce-value">${block.nonce}</div>
      </div>
      
      <div class="detail-item">
        <div class="detail-label">Block Reward</div>
        <div class="detail-value" id="block-reward-value">${block.blockRewardTxt || this.formatGNCValue(block.blockReward)} GNC</div>
      </div>
      
      <div class="detail-item">
        <div class="detail-label">Total Reward</div>
        <div class="detail-value" id="total-reward-value">${block.totalRewardTxt || this.formatGNCValue(block.totalReward)} GNC</div>
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
      <div id="block-transactions-pagination" class="pagination-controls"></div>
    ` : ''}
  `;

        blockDetailsElement.innerHTML = html;

        // Add expand animation class with glitch effect
        blockDetailsElement.classList.add('cyber-expanding', 'cyber-glitch');

        // Remove animation class after animation completes
        setTimeout(() => {
            blockDetailsElement.classList.remove('cyber-expanding', 'cyber-glitch');
        }, 800);

        // Use the CWindow addCopyEvent API method directly to make elements copyable
        this.mInstance.addCopyEvent('block-id-value', 'click');

        // Make miner ID clickable to search for that domain
        const minerIdElement = this.mInstance.getControl('miner-id-value');
        if (minerIdElement) {
            minerIdElement.classList.add('interactive-element');
            minerIdElement.addEventListener('click', () => {
                if (this.callbacks?.onDomainSearch) {
                    this.callbacks.onDomainSearch(block.minerID);
                }
            });
        }

        // And for parent-id-value:
        const parentIdElement = this.mInstance.getControl('parent-id-value');
        if (parentIdElement) {
            parentIdElement.classList.add('interactive-element');
            parentIdElement.addEventListener('click', () => {
                if (this.callbacks?.onViewBlockDetails) {
                    this.callbacks.onViewBlockDetails(block.parentID);
                }
            });
        }

        // GLink copy button for block
        const glinkBtn = this.mInstance.getControl('copy-block-glink-btn');
        if (glinkBtn) {
            glinkBtn.addEventListener('click', () => {
                this.mInstance.copyBlockGLinkToClipboard(block.blockID);
            });
        }

        if (isKeyBlock) {
            this.mInstance.addCopyEvent('difficulty-value', 'click');
            this.mInstance.addCopyEvent('total-difficulty-value', 'click');
        }

        this.mInstance.addCopyEvent('nonce-value', 'click');
        this.mInstance.addCopyEvent('block-reward-value', 'click');
        this.mInstance.addCopyEvent('total-reward-value', 'click');

        // If transactions exist, initialize table
        if (block.transactions && block.transactions.length > 0) {
            const tableElement = $(this.mInstance.getBody).find("#block-transactions-table")[0];
            const blockTransactionsTable = new Tabulator(tableElement, {
                height: "300px",
                layout: "fitColumns",
                data: block.transactions,
                columns: [{
                        title: "Transaction ID",
                        field: "verifiableID",
                        formatter: this.hashFormatter.bind(this)
                    }, {
                        title: "Status",
                        field: "status",
                        formatter: this.statusFormatter.bind(this),
                        width: 120
                    }, {
                        title: "From",
                        field: "sender",
                        formatter: this.hashFormatter.bind(this)
                    }, {
                        title: "To",
                        field: "receiver",
                        formatter: this.hashFormatter.bind(this)
                    }, {
                        title: "Value",
                        field: "value",
                        formatter: this.valueFormatter.bind(this),
                        width: 120
                    }
                ],
                rowClick: (e, row) => {
                    if (this.callbacks?.onViewTransactionDetails) {
                        this.callbacks.onViewTransactionDetails(row.getData().verifiableID);
                    }
                }
            });

            // Add row click handler separately
            blockTransactionsTable.on("rowClick", (e, row) => {
                if (this.callbacks && this.callbacks.onViewBlockDetails) {
                    this.callbacks.onViewTransactionDetails(row.getData().verifiableID);
                }
            });

            // Add cell hover effects
            blockTransactionsTable.on("cellMouseOver", (e, cell) => {
                const value = cell.getValue();
                if (value) {
                    cell.getElement().setAttribute("title", value);
                    cell.getElement().classList.add("hovered-cell");
                }
            });

            blockTransactionsTable.on("cellMouseOut", (e, cell) => {
                cell.getElement().classList.remove("hovered-cell");
            });

            if (block.transactions.length > 10) {
                blockTransactionsTable.setData(block.transactions);
                blockTransactionsTable.setSort("value", "desc");

                const paginationContainer = $(this.mInstance.getBody).find('#block-transactions-pagination')[0];
                if (paginationContainer) {
                    const paginationState = {
                        currentPage: 1,
                        pageSize: 10,
                        totalPages: Math.ceil(block.transactions.length / 10),
                        totalItems: block.transactions.length
                    };

                    this.renderPaginationControls('blockTransactions', paginationState);
                    this.addClientPaginationListeners(paginationContainer, 'blockTransactions', blockTransactionsTable, block.transactions);
                }
            }
        }
    }

    /**
     * Add client-side pagination event listeners
     * @param {HTMLElement} container Pagination container
     * @param {string} section Section identifier
     * @param {Tabulator} table Tabulator instance
     * @param {Array} allData All data for client-side pagination
     */
    addClientPaginationListeners(container, section, table, allData) {
        // Get pagination buttons
        const buttons = container.querySelectorAll('.pagination-button');
        let currentPage = 1;
        let pageSize = 10;

        buttons.forEach(button => {
            button.addEventListener('click', () => {
                if (button.hasAttribute('disabled'))
                    return;

                const pageAction = button.getAttribute('data-page');
                const totalPages = Math.ceil(allData.length / pageSize);

                let newPage = currentPage;

                // Determine the new page based on the button clicked
                switch (pageAction) {
                case 'first':
                    newPage = 1;
                    break;
                case 'prev':
                    newPage = Math.max(1, currentPage - 1);
                    break;
                case 'next':
                    newPage = Math.min(totalPages, currentPage + 1);
                    break;
                case 'last':
                    newPage = totalPages;
                    break;
                default:
                    // If it's a page number
                    if (!isNaN(pageAction)) {
                        newPage = parseInt(pageAction, 10);
                    }
                }

                if (newPage !== currentPage) {
                    currentPage = newPage;

                    // Update table data for the page
                    const startIdx = (currentPage - 1) * pageSize;
                    const pageData = allData.slice(startIdx, startIdx + pageSize);
                    table.setData(pageData);

                    // Update pagination display
                    const paginationState = {
                        currentPage: currentPage,
                        pageSize: pageSize,
                        totalPages: totalPages,
                        totalItems: allData.length
                    };

                    this.renderPaginationControls(section, paginationState);
                    this.addClientPaginationListeners(container, section, table, allData);
                }
            });
        });

        // Page size select
        const pageSizeSelect = container.querySelector('.page-size-select');
        if (pageSizeSelect) {
            pageSizeSelect.addEventListener('change', () => {
                const newSize = parseInt(pageSizeSelect.value, 10);
                if (newSize !== pageSize) {
                    pageSize = newSize;
                    currentPage = 1; // Reset to first page

                    // Update table data for the new page size
                    const pageData = allData.slice(0, pageSize);
                    table.setData(pageData);

                    // Update pagination display
                    const totalPages = Math.ceil(allData.length / pageSize);
                    const paginationState = {
                        currentPage: currentPage,
                        pageSize: pageSize,
                        totalPages: totalPages,
                        totalItems: allData.length
                    };

                    this.renderPaginationControls(section, paginationState);
                    this.addClientPaginationListeners(container, section, table, allData);
                }
            });
        }
    }

    /**
     * Display transaction details in the UI
     * @param {Object} tx Transaction data
     */
    displayTransactionDetails(tx) {
        // Use window-scoped element access to get the transaction details container
        const txDetailsElement = this.mInstance.getControl('transaction-details');
        if (!txDetailsElement)
            return;

        if (!tx) {
            txDetailsElement.innerHTML = '<p>No transaction details available</p>';
            return;
        }

        // First clear any existing content (no animation if replacing)
        if (txDetailsElement.innerHTML.trim()) {
            txDetailsElement.innerHTML = "";
        }

        // Format the transaction data for display
        const html = `
<div class="detail-view fade-in">
  <div class="detail-header">
    <div class="detail-type">Transaction Details</div>
    <div style="display: flex; align-items: center; gap: 10px;">
      <span style="color: ${tx.statusColor || ''};">${tx.status || 'Unknown'}</span>
      <button class="glink-copy-btn" id="copy-tx-glink-btn" title="Share this transaction">
        <i class="fa fa-share-alt"></i> Share
      </button>
    </div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Transaction ID</div>
    <div class="detail-value hash-value" id="tx-id-value">${tx.verifiableID}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Block</div>
    <div class="detail-value">
      ${tx.height > 0 ? `<span class="hash-value" id="tx-block-id-value">${tx.blockID}</span> (Height: ${tx.height})` : 'Pending'}
    </div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Timestamp</div>
    <div class="detail-value">
      ${tx.confirmedTimestamp > 0 ? new Date(tx.confirmedTimestamp * 1000).toLocaleString() : 'Pending'}
      ${tx.unconfirmedTimestamp > 0 ? ` (Broadcasted: ${new Date(tx.unconfirmedTimestamp * 1000).toLocaleString()})` : ''}
    </div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">From</div>
    <div class="detail-value hash-value" id="tx-sender-value">${tx.sender}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">To</div>
    <div class="detail-value hash-value" id="tx-receiver-value">${tx.receiver}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Value</div>
    <div class="detail-value" id="tx-value-value">${tx.valueTxt || this.formatGNCValue(tx.value)}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Transaction Fee</div>
    <div class="detail-value" id="tx-fee-value">${this.formatGNCValue(tx.fee)} </div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Tax</div>
    <div class="detail-value" id="tx-tax-value">${tx.taxTxt || this.formatGNCValue(tx.tax)}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Type</div>
    <div class="detail-value">${this.getTransactionTypeName(tx.type)}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Sacrificed Value</div>
    <div class="detail-value" id="tx-sacrificed-value">${this.formatGNCValue(tx.sacrificedValue)}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Key Height</div>
    <div class="detail-value" id="tx-key-height-value">${tx.keyHeight || 'N/A'}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Size</div>
    <div class="detail-value" id="tx-size-value">${this.mTools.formatByteSize(tx.size && tx.size || 0)}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Nonce</div>
    <div class="detail-value" id="tx-nonce-value">${tx.nonce}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Result</div>
    <div class="detail-value" style="color: ${tx.statusColor || ''};">${CTools.transactionStatusText(tx.result) || 'Unknown'}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">ERG Used / Limit</div>
    <div class="detail-value" id="tx-erg-usage-value">${tx.ERGUsed} / ${tx.ERGLimit}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">ERG Price</div>
    <div class="detail-value" id="tx-erg-price-value">${tx.ERGPrice}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Parsing Result Valid</div>
    <div class="detail-value">${tx.parsingResultValid ? 'Yes' : 'No'}</div>
  </div>
  
  ${tx.getAdditionalAnalysisHints && tx.getAdditionalAnalysisHints().length > 0 ? `
    <div class="detail-item">
      <div class="detail-label">Analysis Hints</div>
      <div class="detail-value">
        <ul>
          ${tx.getAdditionalAnalysisHints().map(hint => `<li>${hint}</li>`).join('')}
        </ul>
      </div>
    </div>
  ` : ''}

 ${tx.log && tx.log.length > 0 ? `
<div class="detail-item">
  <div class="detail-label log-section-header">
    <span>Transaction Log</span>
    <button class="log-toggle-btn" id="tx-log-toggle">
      <span class="toggle-icon">⬇</span>
    </button>
  </div>
  <div class="detail-value log-container" id="tx-log-container" style="display: none;">
    <div class="log-header">
      <span class="log-title">Terminal Output</span>
      <button class="copy-btn" id="copy-log-btn" title="Copy to clipboard">
        <span class="copy-icon">⎘</span>
      </button>
    </div>
    <div class="log-display" id="tx-log-value">
      ${this.formatLogEntries(tx.log)}
    </div>
  </div>
</div>
            ` : ''}

 
${tx.sourceCode ? `
<div class="detail-item">
  <div class="detail-label code-section-header">
    <span>Source Code</span>
    <button class="code-toggle-btn" id="source-code-toggle">
      <span class="toggle-icon">⬇</span>
    </button>
  </div>
  <div class="detail-value code-container" id="source-code-container" style="display: none;">
    <div class="code-header">
      <span class="code-title">GridScript</span>
      <button class="copy-btn" id="copy-source-code-btn" title="Copy to clipboard">
        <span class="copy-icon">⎘</span>
      </button>
    </div>
    <pre class="code-display"><code class="gridscript" id="tx-source-code-value">${this.escapeHtml(tx.sourceCode)}</code></pre>
  </div>
</div>
` : ""}`

            txDetailsElement.innerHTML = html;

        // Add expand animation class with glitch effect
        txDetailsElement.classList.add('cyber-expanding', 'cyber-glitch');

        // Remove animation class after animation completes
        setTimeout(() => {
            txDetailsElement.classList.remove('cyber-expanding', 'cyber-glitch');
        }, 800);

        // Copy Fields Support - BEGIN

        // Make essential fields copyable using CWindow API
        this.mInstance.addCopyEvent('tx-id-value', 'click');
        this.mInstance.addCopyEvent('tx-value-value', 'click');
        this.mInstance.addCopyEvent('tx-fee-value', 'click');
        this.mInstance.addCopyEvent('tx-tax-value', 'click');
        this.mInstance.addCopyEvent('tx-sacrificed-value', 'click');
        this.mInstance.addCopyEvent('tx-key-height-value', 'click');
        this.mInstance.addCopyEvent('tx-size-value', 'click');
        this.mInstance.addCopyEvent('tx-nonce-value', 'click');
        this.mInstance.addCopyEvent('tx-erg-usage-value', 'click');
        this.mInstance.addCopyEvent('tx-erg-price-value', 'click');
        // Copy Fields Support - END

        // Go To Support - BEGIN

        // Make Block ID clickable to view block details
        if (tx.height > 0) {
            const blockIdElement = this.mInstance.getControl('tx-block-id-value');
            if (blockIdElement) {
                blockIdElement.classList.add('interactive-element');
                blockIdElement.addEventListener('click', () => {
                    if (this.callbacks?.onViewBlockDetails) {
                        this.callbacks.onViewBlockDetails(tx.blockID);
                    }
                });
            }
        }

        // Make sender (From) clickable to view domain details
        const senderElement = this.mInstance.getControl('tx-sender-value');
        if (senderElement) {
            senderElement.classList.add('interactive-element');
            senderElement.addEventListener('click', () => {
                if (this.callbacks?.onDomainSearch) {
                    this.callbacks.onDomainSearch(tx.sender);
                }
            });
        }

        // Make receiver (To) clickable to view domain details
        const receiverElement = this.mInstance.getControl('tx-receiver-value');
        if (receiverElement) {
            receiverElement.classList.add('interactive-element');
            receiverElement.addEventListener('click', () => {
                if (this.callbacks?.onDomainSearch) {
                    this.callbacks.onDomainSearch(tx.receiver);
                }
            });
        }

        // Go To Support - END

        // GLink copy button for transaction
        const glinkBtn = this.mInstance.getControl('copy-tx-glink-btn');
        if (glinkBtn) {
            glinkBtn.addEventListener('click', () => {
                this.mInstance.copyTransactionGLinkToClipboard(tx.verifiableID);
            });
        }

        // Make log and source code copyable if they exist
        if (tx.log && tx.log.length > 0) {
            this.mInstance.addCopyEvent('tx-log-value', 'click');
        }
        if (tx.sourceCode) {
            this.mInstance.addCopyEvent('tx-source-code-value', 'click');
        }

        if (tx.sourceCode) {
            // Initialize source code highlighting after the Shadow DOM is updated
            setTimeout(() => {
                this.initSourceCodeHighlighting(
                    'tx-source-code-value',
                    'source-code-toggle',
                    'source-code-container');
            }, 100);
        }

        if (tx.log && tx.log.length > 0) {
            // Initialize transaction log display
            setTimeout(() => {
                this.initLogDisplay(
                    'tx-log-value',
                    'tx-log-toggle',
                    'tx-log-container');
            }, 100);
        }
    }

    /**
     * Display domain details in the UI
     * @param {Object} domain Domain data
     */
    displayDomainDetails(domain) {
        // Use window-scoped element access for the domain details container
        const domainDetailsElement = this.mInstance.getControl('domain-details');
        if (!domainDetailsElement)
            return;

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
    <div style="display: flex; align-items: center; gap: 10px;">
      <span>Transactions: ${domain.txCount || 0}</span>
      <button class="glink-copy-btn" id="copy-domain-glink-btn" title="Share this domain">
        <i class="fa fa-share-alt"></i> Share
      </button>
    </div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Domain</div>
    <div class="detail-value hash-value" id="domain-name-value">${domain.domain}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Balance</div>
    <div class="detail-value" id="domain-balance-value">${domain.balanceTxt || this.formatGNCValue(domain.balance)} </div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Locked Balance</div>
    <div class="detail-value" id="domain-locked-balance-value">${domain.lockedBalanceTxt || this.formatGNCValue(domain.lockedBalance)} </div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Total Received</div>
    <div class="detail-value" id="domain-total-received-value">${domain.txTotalReceivedTxt || this.formatGNCValue(domain.txTotalReceived)} </div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Total Sent</div>
    <div class="detail-value" id="domain-total-sent-value">${domain.txTotalSentTxt || this.formatGNCValue(domain.txTotalSent)} </div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Total Mined</div>
    <div class="detail-value" id="domain-total-mined-value">${domain.GNCTotalMinedTxt || this.formatGNCValue(domain.GNCTotalMined)} </div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Genesis Rewards</div>
    <div class="detail-value" id="domain-genesis-rewards-value">${domain.GNCTotalGenesisRewardsTxt || this.formatGNCValue(domain.GNCTotalGenesisRewards)}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Nonce</div>
    <div class="detail-value" id="domain-nonce-value">${domain.nonce || 0}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Current Perspective</div>
    <div class="detail-value hash-value" id="domain-current-perspective-value">${domain.perspective || 'None'}</div>
  </div>
  
  <div class="detail-item">
    <div class="detail-label">Perspectives Count</div>
    <div class="detail-value" id="domain-perspectives-count-value">${domain.perspectivesCount || 0}</div>
  </div>
  
  ${domain.perspectives && domain.perspectives.length > 0 ? `
    <div class="detail-group">
      <div class="detail-group-header" id="perspectives-header">
        <span>Perspectives</span>
        <span class="toggle-icon">▼</span>
      </div>
      <div class="detail-group-content" id="perspectives-content" style="display: none;">
        <div class="detail-item">
          <div class="detail-value">
            <ul class="perspectives-list">
              ${domain.perspectives.map((p, index) => 
                `<li><span class="hash-value" id="domain-perspective-${index}-value">${p}</span></li>`).join('')}
            </ul>
          </div>
        </div>
      </div>
    </div>
  ` : ''}
  
  ${domain.identityToken ? `
    <div class="detail-group">
      <div class="detail-group-header" id="identity-token-header">
        <span>Identity Token</span>
        <span class="toggle-icon">▼</span>
      </div>
      <div class="detail-group-content" id="identity-token-content" style="display: none;">
        <div class="detail-item">
          <div class="detail-label">Type</div>
          <div class="detail-value" id="identity-token-type-value">${domain.identityToken.getType ? this.mTools.getIdentityTokenTypeString(domain.identityToken.getType()) : 'N/A'}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Version</div>
          <div class="detail-value" id="identity-token-version-value">${domain.identityToken.getVersion ? domain.identityToken.getVersion() : 'N/A'}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Nickname</div>
          <div class="detail-value hash-value" id="identity-token-nickname-value">${domain.identityToken.nickname || domain.identityToken.getFriendlyID?.() || 'N/A'}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Public Key</div>
          <div class="detail-value hash-value" id="identity-token-pubkey-value">${domain.identityToken.publicKeyTxt || 'N/A'}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Sender Address</div>
          <div class="detail-value hash-value" id="identity-token-sender-value">${domain.identityToken.getSenderAddress?.() || 'N/A'}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Rank</div>
          <div class="detail-value" id="identity-token-rank-value">${domain.identityToken.getRank?.() || 'N/A'}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Stake</div>
          <div class="detail-value" id="identity-token-stake-value">${domain.identityToken.stakeTxt || this.formatGNCValue(domain.identityToken.getConsumedCoins?.() || 0) + ' GNC'}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">TX Receipt ID</div>
          <div class="detail-value hash-value" id="identity-token-txreceipt-value">${domain.identityToken.getTXReceiptID?.() || 'N/A'}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Signature</div>
          <div class="detail-value hash-value" id="identity-token-signature-value">${domain.identityToken.getSignature?.() || 'N/A'}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">PoW</div>
          <div class="detail-value" id="identity-token-pow-value">${domain.identityToken.getPow?.() || 'N/A'}</div>
        </div>
      </div>
    </div>
  ` : ''}
  
  ${domain.securityInfo ? `
    <div class="detail-group">
      <div class="detail-group-header" id="security-info-header">
        <span>Operator Security Info</span>
        <span class="toggle-icon">▼</span>
      </div>
      <div class="detail-group-content" id="security-info-content" style="display: none;">
        <div class="detail-item">
          <div class="detail-label">Confidence Level</div>
          <div class="detail-value" id="security-confidence-level-value">${domain.securityInfo.getConfidenceLevel ? (domain.securityInfo.getConfidenceLevel() * 100).toFixed(2) + '%' : 'N/A'}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Operator ID</div>
          <div class="detail-value hash-value" id="security-operator-id-value">${domain.securityInfo.getOperatorID?.() || 'N/A'}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Timestamp Manipulation Count</div>
          <div class="detail-value" id="security-timestamp-manipulation-value">${domain.securityInfo.getTimestampManipulationCount?.() || 0}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">PoW Wave Attack Count</div>
          <div class="detail-value" id="security-pow-wave-attack-value">${domain.securityInfo.getPowWaveAttackCount?.() || 0}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Blocks Mined</div>
          <div class="detail-value" id="security-blocks-mined-value">${domain.securityInfo.getBlocksMined?.() || 0}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Blocks Mined During Difficulty Decrease</div>
          <div class="detail-value" id="security-blocks-mined-decrease-value">${domain.securityInfo.getBlocksMinedDuringDifficultyDecrease?.() || 0}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Blocks Mined During Difficulty Increase</div>
          <div class="detail-value" id="security-blocks-mined-increase-value">${domain.securityInfo.getBlocksMinedDuringDifficultyIncrease?.() || 0}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Blocks Mined During Anomalous Intervals</div>
          <div class="detail-value" id="security-blocks-mined-anomalous-value">${domain.securityInfo.getBlocksMinedDuringAnomalousIntervals?.() || 0}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Last Reported At Height</div>
          <div class="detail-value" id="security-last-reported-height-value">${domain.securityInfo.getLastReportedAtHeight?.() || 0}</div>
        </div>
        <div class="detail-item">
          <div class="detail-label">Global Anomaly Count</div>
          <div class="detail-value" id="security-global-anomaly-count-value">${domain.securityInfo.getGlobalAnomalyCount?.() || 0}</div>
        </div>
        ${domain.securityInfo.getAdditionalAnalysisHints ? `
          <div class="detail-item">
            <div class="detail-label">Analysis Hints</div>
            <div class="detail-value">
              <ul class="analysis-hints-list">
                ${domain.securityInfo.getAdditionalAnalysisHints().map((hint, index) => 
                `<li class="security-hint" id="security-hint-${index}-value">${hint}</li>`).join('')}
              </ul>
            </div>
          </div>
        ` : ''}
      </div>
    </div>
  ` : ''}
</div>

<h3>Transaction History</h3>
<div class="data-table" id="domain-history-table"></div>
<div id="domain-history-pagination" class="pagination-controls"></div>

<style>
  .detail-group {
    border: 1px solid var(--panel-border);
    border-radius: 8px;
    margin: 1rem 0;
    overflow: hidden;
  }
  
  .detail-group-header {
	color: #8059bb;
    background-color: rgba(61, 110, 255, 0.1);
    padding: 0.75rem 1rem;
    display: flex;
    justify-content: space-between;
    align-items: center;
    cursor: pointer;
    font-weight: bold;
    border-bottom: 1px solid var(--panel-border);
  }
  
  .detail-group-header:hover {
    background-color: rgba(61, 110, 255, 0.2);
  }
  
  .detail-group-content {
    padding: 0.5rem;
    background-color: rgba(0, 0, 0, 0.2);
  }
  
  .toggle-icon {
    transition: transform 0.3s ease;
  }
  
  .toggle-icon.open {
    transform: rotate(180deg);
  }
  
  .analysis-hints-list, .perspectives-list {
    margin: 0;
    padding-left: 1.5rem;
  }
  
  .analysis-hints-list li, .perspectives-list li {
    margin-bottom: 0.5rem;
  }
  
</style>
`;

        domainDetailsElement.innerHTML = html;

        // Make essential fields copyable using CWindow API
        this.mInstance.addCopyEvent('domain-name-value', 'click');
        this.mInstance.addCopyEvent('domain-balance-value', 'click');
        this.mInstance.addCopyEvent('domain-locked-balance-value', 'click');
        this.mInstance.addCopyEvent('domain-total-received-value', 'click');
        this.mInstance.addCopyEvent('domain-total-sent-value', 'click');
        this.mInstance.addCopyEvent('domain-total-mined-value', 'click');
        this.mInstance.addCopyEvent('domain-genesis-rewards-value', 'click');
        this.mInstance.addCopyEvent('domain-nonce-value', 'click');

        // GLink copy button for domain
        const glinkBtn = this.mInstance.getControl('copy-domain-glink-btn');
        if (glinkBtn) {
            glinkBtn.addEventListener('click', () => {
                this.mInstance.copyDomainGLinkToClipboard(domain.domain);
            });
        }

        // Make perspectives copyable if they exist
        if (domain.perspectives && domain.perspectives.length > 0) {
            // Set up collapsible behavior for perspectives
            const perspectivesHeader = this.mInstance.getControl('perspectives-header');
            const perspectivesContent = this.mInstance.getControl('perspectives-content');
            const perspectivesToggleIcon = perspectivesHeader.querySelector('.toggle-icon');

            if (perspectivesHeader && perspectivesContent) {
                perspectivesHeader.addEventListener('click', () => {
                    const isOpen = perspectivesContent.style.display !== 'none';
                    perspectivesContent.style.display = isOpen ? 'none' : 'block';
                    if (perspectivesToggleIcon) {
                        if (isOpen) {
                            perspectivesToggleIcon.classList.remove('open');
                        } else {
                            perspectivesToggleIcon.classList.add('open');
                        }
                    }
                });
            }

            // Make each individual perspective copyable
            domain.perspectives.forEach((_, index) => {
                this.mInstance.addCopyEvent(`domain-perspective-${index}-value`, 'click');
            });

            // Make current perspective copyable
            this.mInstance.addCopyEvent('domain-current-perspective-value', 'click');
            this.mInstance.addCopyEvent('domain-perspectives-count-value', 'click');
        }

        // Make identity token fields copyable if they exist
        if (domain.identityToken) {
            // Set up collapsible behavior for identity token
            const tokenHeader = this.mInstance.getControl('identity-token-header');
            const tokenContent = this.mInstance.getControl('identity-token-content');
            const tokenToggleIcon = tokenHeader.querySelector('.toggle-icon');

            if (tokenHeader && tokenContent) {
                tokenHeader.addEventListener('click', () => {
                    const isOpen = tokenContent.style.display !== 'none';
                    tokenContent.style.display = isOpen ? 'none' : 'block';
                    if (tokenToggleIcon) {
                        if (isOpen) {
                            tokenToggleIcon.classList.remove('open');
                        } else {
                            tokenToggleIcon.classList.add('open');
                        }
                    }
                });
            }

            // Make identity token fields copyable
            this.mInstance.addCopyEvent('identity-token-type-value', 'click');
            this.mInstance.addCopyEvent('identity-token-version-value', 'click');
            this.mInstance.addCopyEvent('identity-token-nickname-value', 'click');
            this.mInstance.addCopyEvent('identity-token-pubkey-value', 'click');
            this.mInstance.addCopyEvent('identity-token-sender-value', 'click');
            this.mInstance.addCopyEvent('identity-token-rank-value', 'click');
            this.mInstance.addCopyEvent('identity-token-stake-value', 'click');
            this.mInstance.addCopyEvent('identity-token-txreceipt-value', 'click');
            this.mInstance.addCopyEvent('identity-token-signature-value', 'click');
            this.mInstance.addCopyEvent('identity-token-pow-value', 'click');
        }

        // Make security info fields copyable if they exist
        if (domain.securityInfo) {
            // Set up collapsible behavior for security info
            const securityHeader = this.mInstance.getControl('security-info-header');
            const securityContent = this.mInstance.getControl('security-info-content');
            const securityToggleIcon = securityHeader.querySelector('.toggle-icon');

            if (securityHeader && securityContent) {
                securityHeader.addEventListener('click', () => {
                    const isOpen = securityContent.style.display !== 'none';
                    securityContent.style.display = isOpen ? 'none' : 'block';
                    if (securityToggleIcon) {
                        if (isOpen) {
                            securityToggleIcon.classList.remove('open');
                        } else {
                            securityToggleIcon.classList.add('open');
                        }
                    }
                });
            }

            // Make security info fields copyable
            this.mInstance.addCopyEvent('security-confidence-level-value', 'click');
            this.mInstance.addCopyEvent('security-operator-id-value', 'click');
            this.mInstance.addCopyEvent('security-timestamp-manipulation-value', 'click');
            this.mInstance.addCopyEvent('security-pow-wave-attack-value', 'click');
            this.mInstance.addCopyEvent('security-blocks-mined-value', 'click');
            this.mInstance.addCopyEvent('security-blocks-mined-decrease-value', 'click');
            this.mInstance.addCopyEvent('security-blocks-mined-increase-value', 'click');
            this.mInstance.addCopyEvent('security-blocks-mined-anomalous-value', 'click');
            this.mInstance.addCopyEvent('security-last-reported-height-value', 'click');
            this.mInstance.addCopyEvent('security-global-anomaly-count-value', 'click');

            // Make each security hint copyable
            if (domain.securityInfo.getAdditionalAnalysisHints) {
                domain.securityInfo.getAdditionalAnalysisHints().forEach((_, index) => {
                    this.mInstance.addCopyEvent(`security-hint-${index}-value`, 'click');
                });
            }
        }

        // Reset to domain history view mode
        this.resetToDomainHistoryView();
    }
    /**
     * Display search results in the UI
     * @param {Array} results Search results array
     * @param {string} query Search query
     */
    displaySearchResults(results, query) {
        if (!results || results.length === 0) {
            this.showError(`No results found for: ${query}`);
            return;
        }

        // Count the number of each type of result
        let blockCount = 0;
        let transactionCount = 0;
        let domainCount = 0;
        const tools = CTools.getInstance();

        // Count each type
        results.forEach(item => {
            const type = tools.getMetaObjectType(item);
            if (type === eSearchResultElemType.BLOCK) {
                blockCount++;
            } else if (type === eSearchResultElemType.TRANSACTION) {
                transactionCount++;
            } else if (type === eSearchResultElemType.DOMAIN) {
                domainCount++;
            }
        });

        // Check if all results are of the same type
        const totalCount = blockCount + transactionCount + domainCount;

        if (blockCount === totalCount) {
            // All results are blocks
            this.switchToSection('blocks');
            if (this.tables.blocks) {
                // Convert to plain objects if needed
                const plainBlocks = this.mTools.convertBlockchainMetaToPlainObject(results);
                this.tables.blocks.setData(plainBlocks);
            }
        } else if (transactionCount === totalCount) {
            // All results are transactions
            this.switchToSection('transactions');
            if (this.tables.transactions) {
                // Convert to plain objects if needed
                const plainTransactions = this.mTools.convertBlockchainMetaToPlainObject(results);
                this.tables.transactions.setData(plainTransactions);
            }
        } else if (domainCount === totalCount) {
            // All results are domains
            this.switchToSection('domains');

            // If there's only one domain, view it directly
            if (results.length === 1 && this.callbacks && this.callbacks.onDomainSearch) {
                this.callbacks.onDomainSearch(results[0].domain);
            } else {
                // Display a list of domains
                this.showSearchResultsOverview(results, query);
            }
        } else {
            // Mixed results or unknown types, show overview
            this.showSearchResultsOverview(results, query);
        }
    }

    /**
     * Update UI to show market data view
     */
    showMarketDataView() {
        // Update table columns to show relevant market data


        if (this.tables.domainHistory) {
            this.tables.domainHistory.setColumns([{
                        title: "Domain",
                        field: "domain",
                        formatter: this.hashFormatter,
                        widthGrow: 2
                    }, {
                        title: "Balance",
                        field: "balance",
                        formatter: (cell) => {
                            const data = cell.getRow().getData();
                            return data.balanceTxt || this.valueFormatter(cell);
                        },
                        sorter: this.bigIntSorter, // Explicitly use our custom sorter
                        width: 130
                    }, {
                        title: "Received",
                        field: "txTotalReceived",
                        formatter: (cell) => {
                            const data = cell.getRow().getData();
                            return data.txTotalReceivedTxt || this.valueFormatter(cell);
                        },
                        sorter: this.bigIntSorter, // Explicitly use our custom sorter
                        width: 130
                    }, {
                        title: "Sent",
                        field: "txTotalSent",
                        formatter: (cell) => {
                            const data = cell.getRow().getData();
                            return data.txTotalSentTxt || this.valueFormatter(cell);
                        },
                        sorter: this.bigIntSorter, // Explicitly use our custom sorter
                        width: 130
                    }, {
                        title: "Locked",
                        field: "lockedBalance",
                        formatter: (cell) => {
                            const data = cell.getRow().getData();
                            return data.lockedBalanceTxt || this.valueFormatter(cell);
                        },
                        sorter: this.bigIntSorter, // Explicitly use our custom sorter
                        width: 120
                    }, {
                        title: "Tx In Count",
                        field: "txInCount",
                        sorter: "number",
                        // width: 100
                    }, {
                        title: "Tx Out Count",
                        field: "txOutCount",
                        sorter: "number",
                        //width: 100
                    }, {
                        title: "Tx Count",
                        field: "txCount",
                        sorter: "number",
                        width: 100
                    }, {
                        title: "Mined",
                        field: "GNCTotalMined",
                        formatter: (cell) => {
                            const data = cell.getRow().getData();
                            return data.getGNCTotalMinedTxt || this.valueFormatter(cell);
                        },
                        sorter: this.bigIntSorter, // Explicitly use our custom sorter
                        width: 120
                    },
                ]);

            // Apply server-side sort params to client table
            const sortParams = this.mInstance.dataManager.lastSortParams.market;
            if (sortParams && sortParams.clientParams && sortParams.clientParams.field && sortParams.clientParams.dir) {
                this.mInstance.ui.tables.domainHistory.setSort(sortParams.clientParams.field, sortParams.clientParams.dir);
                // Force table redraw to ensure indicators are rendered correctly
                // Get the column definition object for the sort field
                const column = this.mInstance.ui.tables.domainHistory.getColumn(sortParams.clientParams.field);

                // if (column) {
                // Use toggleColumnSort which properly updates the header UI
                //   this.ui.tables.domainHistory.toggleColumnSort(
                //        column,
                //       sortParams.clientParams.dir);
                // } else {
                // Fallback to normal setSort if column not found
                this.mInstance.ui.tables.domainHistory.setSort(
                    sortParams.clientParams.field,
                    sortParams.clientParams.dir);
                //}
            }

            // Rest of the method remains unchanged...
            // Update row click handler for market data view
            this.tables.domainHistory.off("rowClick"); // Remove existing handler
            this.tables.domainHistory.on("rowClick", (e, row) => {
                const domain = row.getData().domain;
                if (domain && this.callbacks?.onDomainSearch) {
                    this.callbacks.onDomainSearch(domain);
                }
            });

            // Flag in paginationState that we're in market data mode
            if (this.paginationState && this.paginationState.domainHistory) {
                this.paginationState.domainHistory.isMarketDataView = true;
            }
        }
    }
    /**
     * Show search results overview for mixed result types
     * @param {Array} results Search results
     * @param {string} query Search query
     */
    showSearchResultsOverview(results, query) {
        this.switchToSection('dashboard');

        // Get the dashboard section using window-scoped element access
        const dashboardSection = this.mInstance.getControl('dashboard-section');
        if (!dashboardSection)
            return;

        // Store original dashboard content if not already stored
        if (!this._originalDashboardContent) {
            this._originalDashboardContent = dashboardSection.innerHTML;
        }

        // Create a search results container
        const searchResultsHTML = `
        <div class="search-results fade-in">
            <div class="search-header">
                <h2>Search Results for "${query}"</h2>
                <button class="search-button" id="clear-search-results">Clear Results</button>
            </div>
            
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
            
            <div id="search-pagination" class="pagination-controls"></div>
        </div>
    `;

        // Replace dashboard content with search results
        dashboardSection.innerHTML = searchResultsHTML;

        // Temporarily store tables for this search session
        this.searchTables = {};

        // Initialize tables for each result type
        if (this.hasBlockResults(results)) {
            const tableElement = this.mInstance.getControl("search-results-blocks");
            if (tableElement) {
                this.searchTables.blocks = new Tabulator(tableElement, {
                    height: "300px",
                    layout: "fitColumns",
                    data: this.getBlockResults(results),
                    columns: [{
                            title: "Height",
                            field: "height",
                            sorter: "number",
                            headerSortStartingDir: "desc",
                            width: 100
                        }, {
                            title: "Block ID",
                            field: "blockID",
                            formatter: this.hashFormatter
                        }, {
                            title: "Miner",
                            field: "minerID",
                            formatter: this.hashFormatter
                        }, {
                            title: "Time",
                            field: "solvedAt",
                            formatter: this.timestampFormatter,
                            width: 180
                        }, {
                            title: "Transactions",
                            field: "transactionsCount",
                            sorter: "number",
                            width: 120
                        }
                    ]
                });

                // Add row click handler separately
                this.searchTables.blocks.on("rowClick", (e, row) => {
                    if (this.callbacks && this.callbacks.onViewBlockDetails) {
                        this.callbacks.onViewBlockDetails(row.getData().blockID);
                    }
                });

                // Add cell hover effects
                this.searchTables.blocks.on("cellMouseOver", (e, cell) => {
                    const value = cell.getValue();
                    if (value) {
                        cell.getElement().setAttribute("title", value);
                        cell.getElement().classList.add("hovered-cell");
                    }
                });

                this.searchTables.blocks.on("cellMouseOut", (e, cell) => {
                    cell.getElement().classList.remove("hovered-cell");
                });
            }
        }

        if (this.hasTransactionResults(results)) {
            const tableElement = this.mInstance.getControl("search-results-transactions");
            if (tableElement) {
                this.searchTables.transactions = new Tabulator(tableElement, {
                    height: "300px",
                    layout: "fitColumns",
                    data: this.getTransactionResults(results),
                    columns: [{
                            title: "Transaction ID",
                            field: "verifiableID",
                            formatter: this.hashFormatter.bind(this)
                        }, {
                            title: "Status",
                            field: "status",
                            formatter: this.statusFormatter.bind(this),
                            width: 120
                        }, {
                            title: "From",
                            field: "sender",
                            formatter: this.hashFormatter.bind(this)
                        }, {
                            title: "To",
                            field: "receiver",
                            formatter: this.hashFormatter.bind(this)
                        }, {
                            title: "Value",
                            field: "value",
                            formatter: this.valueFormatter.bind(this),
                            width: 120
                        }, {
                            title: "Time",
                            field: "time",
                            formatter: this.timestampFormatter.bind(this),
                            width: 180
                        }
                    ]
                });

                // Add row click handler separately
                this.searchTables.transactions.on("rowClick", (e, row) => {
                    if (this.callbacks && this.callbacks.onViewTransactionDetails) {
                        this.callbacks.onViewTransactionDetails(row.getData().verifiableID);
                    }
                });

                // Add cell hover effects
                this.searchTables.transactions.on("cellMouseOver", (e, cell) => {
                    const value = cell.getValue();
                    if (value) {
                        cell.getElement().setAttribute("title", value);
                        cell.getElement().classList.add("hovered-cell");
                    }
                });

                this.searchTables.transactions.on("cellMouseOut", (e, cell) => {
                    cell.getElement().classList.remove("hovered-cell");
                });
            }
        }

        if (this.hasDomainResults(results)) {
            const tableElement = this.mInstance.getControl("search-results-domains");
            if (tableElement) {
                this.searchTables.domains = new Tabulator(tableElement, {
                    height: "200px",
                    layout: "fitColumns",
                    data: this.getDomainResults(results),
                    columns: [{
                            title: "Domain",
                            field: "domain",
                            formatter: this.hashFormatter
                        }, {
                            title: "Transactions",
                            field: "txCount",
                            sorter: "number",
                            width: 120
                        }, {
                            title: "Balance",
                            field: "balance",
                            formatter: this.valueFormatter.bind(this),
                            width: 150
                        }
                    ]
                });

                // Add row click handler separately
                this.searchTables.domains.on("rowClick", (e, row) => {
                    if (this.callbacks && this.callbacks.onDomainSearch) {
                        this.callbacks.onDomainSearch(row.getData().domain);
                    }
                });

                // Add cell hover effects
                this.searchTables.domains.on("cellMouseOver", (e, cell) => {
                    const value = cell.getValue();
                    if (value) {
                        cell.getElement().setAttribute("title", value);
                        cell.getElement().classList.add("hovered-cell");
                    }
                });

                this.searchTables.domains.on("cellMouseOut", (e, cell) => {
                    cell.getElement().classList.remove("hovered-cell");
                });
            }
        }

        // Add pagination controls
        const paginationContainer = this.mInstance.getControl('search-pagination');
        if (paginationContainer && this.paginationState.search) {
            this.renderPaginationControls('search', this.paginationState.search);
        }

        // Add click handler for clear results button
        const clearButton = this.mInstance.getControl('clear-search-results');
        if (clearButton) {
            clearButton.addEventListener('click', () => this.clearSearchResults());
        }
    }

    /**
     * Clears search results and restores original dashboard content
     */
    clearSearchResults() {
        // Get the dashboard section
        const dashboardSection = this.mInstance.getControl('dashboard-section');
        if (!dashboardSection || !this._originalDashboardContent)
            return;

        // Destroy all search tables to prevent memory leaks
        if (this.searchTables) {
            Object.values(this.searchTables).forEach(table => {
                if (table && typeof table.destroy === 'function') {
                    table.destroy();
                }
            });
            this.searchTables = {};
        }

        // Restore original dashboard content
        dashboardSection.innerHTML = this._originalDashboardContent;

        // Reinitialize dashboard charts if needed
        if (this.ui && typeof this.ui.updateTransactionsChart === 'function' &&
            typeof this.ui.updateDailyStatsChart === 'function') {
            this.ui.updateTransactionsChart(this.cache.transactionDailyStats);
            this.ui.updateDailyStatsChart(this.cache.transactionDailyStats);
        }
    }
    // Filters - BEGIN

    /**
     * Check if results contain blocks
     * @param {Array} results Search results
     * @returns {boolean} Has block results
     */
    hasBlockResults(results) {
        const tools = CTools.getInstance();
        return results.some(r =>
            tools.getMetaObjectType(r) === this.enums.eSearchResultElemType.BLOCK);
    }

    /**
     * Get block results
     * @param {Array} results Search results
     * @returns {Array} Block results
     */
    getBlockResults(results) {
        const tools = CTools.getInstance();
        return results.filter(r =>
            tools.getMetaObjectType(r) === this.enums.eSearchResultElemType.BLOCK);
    }

    /**
     * Check if results contain transactions
     * @param {Array} results Search results
     * @returns {boolean} Has transaction results
     */
    hasTransactionResults(results) {
        const tools = CTools.getInstance();
        return results.some(r =>
            tools.getMetaObjectType(r) === this.enums.eSearchResultElemType.TRANSACTION);
    }

    /**
     * Get transaction results
     * @param {Array} results Search results
     * @returns {Array} Transaction results
     */
    getTransactionResults(results) {
        const tools = CTools.getInstance();
        return results.filter(r =>
            tools.getMetaObjectType(r) === this.enums.eSearchResultElemType.TRANSACTION);
    }

    /**
     * Check if results contain domains
     * @param {Array} results Search results
     * @returns {boolean} Has domain results
     */
    hasDomainResults(results) {
        const tools = CTools.getInstance();
        return results.some(r =>
            tools.getMetaObjectType(r) === this.enums.eSearchResultElemType.DOMAIN);
    }

    /**
     * Get domain results
     * @param {Array} results Search results
     * @returns {Array} Domain results
     */
    getDomainResults(results) {
        const tools = CTools.getInstance();
        return results.filter(r =>
            tools.getMetaObjectType(r) === this.enums.eSearchResultElemType.DOMAIN);
    }

    // Filters - END

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
        case 0:
            return "TRANSFER";
        case 1:
            return "BLOCK REWARD";
        case 2:
            return "OFF CHAIN";
        case 3:
            return "OFF CHAIN CASH OUT";
        case 4:
            return "CONTRACT";
        default:
            return "UNKNOWN";
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
        if (!value)
            return "";

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
        if (!value)
            return "";

        // Get row data to access the result code
        const row = cell.getRow();
        const rowData = row.getData();

        // Use either the stored result value or infer from the status text
        let color;
        if (rowData.result !== undefined) {
            // Use the original result value
            color = CTools.transactionStatusColor(rowData.result);
        } else if (rowData.statusColor) {
            // Use pre-calculated status color if available
            color = rowData.statusColor;
        } else {
            // Try to infer from the status text
            color = this.inferStatusColor(value);
        }

        return `<span style="color: ${color}; font-weight: bold;">${value}</span>`;
    }

    /**
     * Infer status color from status text when result code is not available
     * @param {string} status The status text
     * @returns {string} Color code
     */
    inferStatusColor(status) {
        if (typeof status !== 'string')
            return "#000000";

        const statusLower = status.toLowerCase();

        if (statusLower.includes('finalized') && statusLower.includes('safe')) {
            return "#0fd41c"; // Green for safe finalized
        } else if (statusLower.includes('finalized') && statusLower.includes('pending')) {
            return "orange"; // Orange for pending finalized
        } else if (statusLower.includes('succeeded') || statusLower.includes('valid')) {
            return "#0fd41c"; // Green for success
        } else if (statusLower.includes('pending') || statusLower.includes('scheduled')) {
            return "#fa3497"; // Pink for pending
        } else if (statusLower.includes('invalid') || statusLower.includes('error') ||
            statusLower.includes('failed') || statusLower.includes('forked')) {
            return "#db7695"; // Red for failures
        } else if (statusLower.includes('broadcast')) {
            return "orange"; // Orange for broadcast
        }

        return "#000000"; // Default black
    }

    /**
     * Format value (currency) for display in tables
     * @param {Object} cell Cell value
     * @returns {string} Formatted value
     */
    valueFormatter(cell) {
        let value = cell.getValue();

        // If value is null or undefined, return empty string
        if (value == null)
            return "";

        // If value is already a string, we assume it's already formatted
        if (typeof value === 'string')
            return value;

        // Try to format the value
        try {
            return this.formatGNCValue(value);
        } catch (e) {
            // If unable to format, return the original value as string
            return String(value);
        }
    }

    /**
     * Format timestamp for display in tables
     * @param {Object} cell Cell value
     * @returns {string} Formatted timestamp
     */
    timestampFormatter(cell) {
        const timestamp = cell.getValue();
        if (!timestamp)
            return "";

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
        if (bytes == null)
            return "";

        return this.mTools.formatByteSize(bytes);
    }

    /**
     * Format a GNC value consistently
     * @param {BigInt|string|number} value The GNC value to format
     * @param {number} precision Number of decimal places to show
     * @returns {string} Formatted GNC value
     */
    formatGNCValue(value, precision = 5) {
        try {
            // Try to use CTools if available
            const tools = CTools.getInstance();
            if (tools && typeof tools.formatGNCValue === 'function') {
                return tools.formatGNCValue(value, precision);
            }

            // Fallback implementation
            if (typeof value === 'bigint') {
                // Basic GNC formatting: 1 GNC = 10^18 atto
                const divisor = 10n ** 18n;
                const integerPart = value / divisor;
                const fractionalPart = value % divisor;

                // Format fractional part with proper padding
                let fractionalStr = fractionalPart.toString().padStart(18, '0');
                fractionalStr = fractionalStr.substring(0, precision);

                return `${integerPart}.${fractionalStr}`;
            } else if (typeof value === 'number') {
                return value.toFixed(precision);
            }

            // If it's already a string or other type, just convert to string
            return String(value);
        } catch (error) {
            console.warn("Error formatting GNC value:", error);
            return String(value);
        }
    }

    /**
     * Handle window resize event
     * @param {number} width Window width
     * @param {number} height Window height
     */

    /**
     * Handle window resize event
     * @param {number} width Window width
     * @param {number} height Window height
     */
    handleResize(width, height) {
        try {
            // Get the active section to only resize visible tables
            const activeSection = this.mInstance.getActiveSection();

            // Determine which tables should be resized based on active section
            const visibleTables = {
                dashboard: ['recentBlocks', 'recentTransactions'],
                blocks: ['blocks'],
                transactions: ['transactions'],
                domains: ['domainHistory'],
                // Add other sections as needed
            }
            [activeSection] || [];

            // Target height based on window size
            const tableHeights = height < 600 ? {
                recentBlocks: 200,
                recentTransactions: 200,
                blocks: 400,
                transactions: 400,
                domainHistory: 300
            }
             : {
                recentBlocks: 300,
                recentTransactions: 300,
                blocks: 600,
                transactions: 600,
                domainHistory: 400
            };

            // Only resize tables that are in the active section
            visibleTables.forEach(tableKey => {
                const table = this.tables[tableKey];
                if (!table)
                    return;

                try {
                    // Check if table's element exists and is properly initialized
                    const tableElement = table.element;
                    if (!tableElement || !tableElement.parentNode ||
                        tableElement.getAttribute('data-tabulator-initialized') !== 'true') {
                        return;
                    }

                    // Check if table is visible
                    const isVisible = window.getComputedStyle(tableElement).display !== 'none';
                    if (!isVisible)
                        return;

                    // Set height and trigger a layout recalculation
                    table.setHeight(tableHeights[tableKey]);

                    // Redraw with a small delay to ensure the DOM has updated
                    setTimeout(() => {
                        try {
                            if (table && typeof table.redraw === 'function') {
                                table.redraw(true);
                            }
                        } catch (innerError) {
                            console.warn(`Failed to redraw table ${tableKey}:`, innerError);
                        }
                    }, 50);
                } catch (tableError) {
                    console.warn(`Error resizing table ${tableKey}:`, tableError);
                }
            });

            // Update charts to fit new size using the stored chart container references
            if (activeSection === 'dashboard' && this.charts.transactionsChart) {
                try {
                    Plotly.relayout(this.charts.transactionsChart, {
                        width: Math.max(300, width - 40), // Ensure minimum width
                        height: 300
                    });
                } catch (e) {
                    console.warn("Error resizing transactions chart:", e);
                }
            }

            if (activeSection === 'statistics' && this.charts.dailyStatsChart) {
                try {
                    Plotly.relayout(this.charts.dailyStatsChart, {
                        width: Math.max(300, width - 40), // Ensure minimum width
                        height: 300
                    });
                } catch (e) {
                    console.warn("Error resizing daily stats chart:", e);
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
        } catch (error) {
            console.error("Error during UI resize:", error);
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

        // Destroy charts using the stored chart container references
        if (this.charts.transactionsChart) {
            try {
                Plotly.purge(this.charts.transactionsChart);
            } catch (e) {
                console.warn("Error purging transactions chart", e);
            }
        }

        if (this.charts.dailyStatsChart) {
            try {
                Plotly.purge(this.charts.dailyStatsChart);
            } catch (e) {
                console.warn("Error purging daily stats chart", e);
            }
        }

        // Clear chart references
        this.charts = {};

        // Clear callbacks
        this.callbacks = null;

        // Clean up search autocomplete
        if (this.searchAutocomplete) {
            this.elements.searchInput?.removeEventListener('keydown', this.searchAutocomplete.keydownHandler);
            this.searchAutocomplete.container?.remove();
        }
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
