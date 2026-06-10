// SignalBoard Dashboard - Frontend Application Logic

class SignalBoardApp {
    constructor() {
        this.apiBase = window.location.origin;
        this.updateInterval = null;
        this.signals = [];
        this.init();
    }

    /**
     * Initialize application
     */
    async init() {
        console.log('[App] Initializing SignalBoard Dashboard...');

        // Load signals
        await this.loadSignals();

        // Load signal slot configuration
        await this.loadConfig();

        // Load system info
        await this.updateSystemInfo();

        // Attach event listeners
        this.attachEventListeners();

        // Start update interval (every 2 seconds)
        this.updateInterval = setInterval(() => {
            this.updateSystemInfo();
        }, 2000);

        console.log('[App] Ready');
    }

    /**
     * Load signals and render grid
     */
    async loadSignals() {
        try {
            const response = await fetch(`${this.apiBase}/api/signals/status`);
            if (!response.ok) throw new Error(`HTTP ${response.status}`);

            const data = await response.json();

            if (data.signals && Array.isArray(data.signals)) {
                this.signals = data.signals;
                this.renderSignalsGrid();
                this.updateStatusBar();
            }
        } catch (error) {
            console.error('[App] Failed to load signals:', error);
            this.showError('Failed to load signals');
        }
    }

    /**
     * Render signals grid
     */
    renderSignalsGrid() {
        const grid = document.getElementById('signals-grid');
        if (!grid) return;

        if (this.signals.length === 0) {
            grid.innerHTML = '<p class="loading">No signals loaded</p>';
            return;
        }

        grid.innerHTML = this.signals
            .map((signal) => this.createSignalCard(signal))
            .join('');

        // Attach aspect button listeners
        document.querySelectorAll('.aspect-btn').forEach((btn) => {
            btn.addEventListener('click', (e) =>
                this.handleAspectChange(e.target)
            );
        });
    }

    /**
     * Create signal card HTML
     */
    createSignalCard(signal) {
        const typeLabel =
            signal.type === 0 ? 'MAIN' : signal.type === 1 ? 'SHUNT' : 'UNKNOWN';
        const displayEmoji = this.getSignalEmoji(signal.type, signal.aspect);

        return `
            <div class="signal-card" data-signal-id="${signal.id}">
                <div class="signal-header">
                    <span class="signal-id">${signal.id}</span>
                    <span class="signal-type">${typeLabel}</span>
                </div>
                <div class="signal-display">${displayEmoji}</div>
                <div class="signal-controls">
                    ${this.createAspectButtons(signal)}
                </div>
            </div>
        `;
    }

    /**
     * Create aspect control buttons
     */
    createAspectButtons(signal) {
        if (signal.type === 0) {
            // MAIN: Red, Yellow, Green
            return `
                <button class="aspect-btn red ${signal.aspect === 0 ? 'active' : ''}" data-aspect="0" title="Red (Stop)">🔴</button>
                <button class="aspect-btn yellow ${signal.aspect === 2 ? 'active' : ''}" data-aspect="2" title="Yellow">🟡</button>
                <button class="aspect-btn green ${signal.aspect === 1 ? 'active' : ''}" data-aspect="1" title="Green (Go)">🟢</button>
            `;
        } else {
            // SHUNT: Red, Yellow+Red, Yellow+Green
            return `
                <button class="aspect-btn red ${signal.aspect === 3 ? 'active' : ''}" data-aspect="3" title="Red (Stop)">🔴</button>
                <button class="aspect-btn yellow ${signal.aspect === 5 ? 'active' : ''}" data-aspect="5" title="Oblique">🟡🔴</button>
                <button class="aspect-btn green ${signal.aspect === 4 ? 'active' : ''}" data-aspect="4" title="Go">🟡🟢</button>
            `;
        }
    }

    /**
     * Get emoji representation of signal aspect
     */
    getSignalEmoji(type, aspect) {
        if (type === 0) {
            // MAIN signal
            switch (aspect) {
                case 0:
                    return '🔴';
                case 1:
                    return '🟢';
                case 2:
                    return '🟡';
                default:
                    return '❓';
            }
        } else {
            // SHUNT signal
            switch (aspect) {
                case 3:
                    return '🔴';
                case 4:
                    return '🟡🟢';
                case 5:
                    return '🟡🔴';
                default:
                    return '❓';
            }
        }
    }

    /**
     * Handle aspect button click
     */
    async handleAspectChange(button) {
        const card = button.closest('.signal-card');
        if (!card) return;

        const signalId = card.dataset.signalId;
        const aspect = parseInt(button.dataset.aspect, 10);

        try {
            const response = await fetch(
                `${this.apiBase}/api/signals/${signalId}/aspect`,
                {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ aspect }),
                }
            );

            if (!response.ok) throw new Error(`HTTP ${response.status}`);

            const data = await response.json();
            if (data.status === 'ok') {
                this.showSuccess(`Signal ${signalId} updated`);
                await this.loadSignals();
            } else {
                this.showError(data.message || 'Failed to update signal');
            }
        } catch (error) {
            console.error('[App] Aspect change failed:', error);
            this.showError('Failed to update signal aspect');
        }
    }

    /**
     * Load signal slot configuration and render editors
     */
    async loadConfig() {
        try {
            const response = await fetch(`${this.apiBase}/api/config/signals`);
            if (!response.ok) throw new Error(`HTTP ${response.status}`);

            const data = await response.json();
            if (data.slots && Array.isArray(data.slots)) {
                this.renderConfigGrid(data.slots);
            }
        } catch (error) {
            console.error('[App] Failed to load config:', error);
            this.showError('Failed to load signal configuration');
        }
    }

    /**
     * Render configuration slot editors
     */
    renderConfigGrid(slots) {
        const grid = document.getElementById('config-grid');
        if (!grid) return;

        grid.innerHTML = slots.map((slot) => this.createSlotCard(slot)).join('');

        grid.querySelectorAll('.slot-save-btn').forEach((btn) => {
            btn.addEventListener('click', () =>
                this.saveSlot(parseInt(btn.dataset.index, 10))
            );
        });
        grid.querySelectorAll('.slot-clear-btn').forEach((btn) => {
            btn.addEventListener('click', () =>
                this.clearSlot(parseInt(btn.dataset.index, 10))
            );
        });
    }

    /**
     * Create slot editor card HTML
     */
    createSlotCard(slot) {
        const i = slot.index;
        const configured = slot.configured;
        const id = configured ? slot.id : '';
        const type = configured ? slot.type : 0;
        const pins = configured ? slot.pins : [i * 3, i * 3 + 1, i * 3 + 2];
        const brightness = configured ? slot.brightness : [4095, 4095, 4095];
        const colors = ['Red', 'Yellow', 'Green'];

        const channelRows = colors
            .map(
                (color, c) => `
                <div class="slot-channel-row">
                    <span class="slot-channel-label">${color}</span>
                    <select id="slot-${i}-pin-${c}" class="slot-input slot-pin">
                        ${this.createChannelOptions(pins[c])}
                    </select>
                    <input type="number" id="slot-${i}-br-${c}"
                           class="slot-input slot-brightness"
                           min="0" max="4095" value="${brightness[c]}"
                           title="Brightness (0-4095)">
                </div>`
            )
            .join('');

        return `
            <div class="slot-card ${configured ? 'configured' : 'empty'}" data-index="${i}">
                <div class="slot-header">
                    <span class="slot-title">Slot ${i + 1}</span>
                    <span class="slot-badge">${configured ? 'CONFIGURED' : 'EMPTY'}</span>
                </div>
                <div class="slot-fields">
                    <div class="slot-row">
                        <label for="slot-${i}-id">Signal ID</label>
                        <input type="text" id="slot-${i}-id" class="slot-input"
                               maxlength="32" placeholder="e.g. sg${i + 1}" value="${id}">
                    </div>
                    <div class="slot-row">
                        <label for="slot-${i}-type">Type</label>
                        <select id="slot-${i}-type" class="slot-input">
                            <option value="0" ${type === 0 ? 'selected' : ''}>MAIN (3-aspect)</option>
                            <option value="1" ${type === 1 ? 'selected' : ''}>SHUNT (marmotta)</option>
                        </select>
                    </div>
                    <div class="slot-channels-header">
                        <span></span><span>Channel</span><span>Brightness</span>
                    </div>
                    ${channelRows}
                </div>
                <div class="slot-actions">
                    <button class="btn btn-primary slot-save-btn" data-index="${i}">💾 Save</button>
                    <button class="btn btn-danger slot-clear-btn" data-index="${i}"
                            ${configured ? '' : 'disabled'}>🗑️ Clear</button>
                </div>
            </div>
        `;
    }

    /**
     * Create PCA9685 channel <option> list (0-15)
     */
    createChannelOptions(selected) {
        let options = '';
        for (let ch = 0; ch < 16; ch++) {
            options += `<option value="${ch}" ${ch === selected ? 'selected' : ''}>CH${ch}</option>`;
        }
        return options;
    }

    /**
     * Save slot configuration
     */
    async saveSlot(index) {
        const id = document.getElementById(`slot-${index}-id`).value.trim();
        if (!id) {
            this.showError('Signal ID is required');
            return;
        }

        const type = parseInt(
            document.getElementById(`slot-${index}-type`).value, 10);
        const pins = [];
        const brightness = [];
        for (let c = 0; c < 3; c++) {
            pins.push(parseInt(
                document.getElementById(`slot-${index}-pin-${c}`).value, 10));
            const br = parseInt(
                document.getElementById(`slot-${index}-br-${c}`).value, 10);
            brightness.push(Math.min(4095, Math.max(0, isNaN(br) ? 4095 : br)));
        }

        if (new Set(pins).size !== 3) {
            this.showError('Channels must be 3 distinct values');
            return;
        }

        try {
            const response = await fetch(
                `${this.apiBase}/api/config/signals/${index}`,
                {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ id, type, pins, brightness }),
                }
            );
            const data = await response.json();

            if (response.ok && data.status === 'ok') {
                this.showSuccess(`Slot ${index + 1} saved (${id})`);
                await this.loadConfig();
                await this.loadSignals();
            } else {
                this.showError(data.message || data.error || 'Save failed');
            }
        } catch (error) {
            console.error('[App] Save slot failed:', error);
            this.showError('Failed to save slot configuration');
        }
    }

    /**
     * Clear slot configuration
     */
    async clearSlot(index) {
        if (!confirm(`Clear slot ${index + 1} configuration?`)) {
            return;
        }

        try {
            const response = await fetch(
                `${this.apiBase}/api/config/signals/${index}`,
                { method: 'DELETE' }
            );
            const data = await response.json();

            if (response.ok && data.status === 'ok') {
                this.showSuccess(`Slot ${index + 1} cleared`);
                await this.loadConfig();
                await this.loadSignals();
            } else {
                this.showError(data.error || 'Clear failed');
            }
        } catch (error) {
            console.error('[App] Clear slot failed:', error);
            this.showError('Failed to clear slot');
        }
    }

    /**
     * Upload an OTA image (firmware or filesystem)
     * Uses XHR for upload progress events.
     */
    uploadOta(endpoint, fileInputId, label) {
        const input = document.getElementById(fileInputId);
        const file = input?.files?.[0];

        if (!file) {
            this.showError(`Select a ${label} .bin file first`);
            return;
        }
        if (!confirm(`Upload ${file.name} as new ${label}? The device will reboot.`)) {
            return;
        }

        const progress = document.getElementById('ota-progress');
        const bar = document.getElementById('ota-progress-bar');
        const text = document.getElementById('ota-progress-text');
        progress.hidden = false;

        const form = new FormData();
        form.append('update', file, file.name);

        const xhr = new XMLHttpRequest();
        xhr.open('POST', `${this.apiBase}${endpoint}`);

        xhr.upload.addEventListener('progress', (e) => {
            if (e.lengthComputable) {
                const pct = Math.round((e.loaded / e.total) * 100);
                bar.style.width = pct + '%';
                text.textContent = pct + '%';
            }
        });

        xhr.addEventListener('load', () => {
            progress.hidden = true;
            bar.style.width = '0%';

            if (xhr.status === 200) {
                this.showSuccess(`${label} updated, device rebooting...`);
                setTimeout(() => window.location.reload(), 8000);
            } else if (xhr.status === 401) {
                this.showError('Authentication required');
            } else {
                this.showError(`${label} update failed`);
            }
        });

        xhr.addEventListener('error', () => {
            progress.hidden = true;
            this.showError('Upload failed (connection error)');
        });

        xhr.send(form);
    }

    /**
     * Update system information
     */
    async updateSystemInfo() {
        try {
            const response = await fetch(`${this.apiBase}/api/system`);
            if (!response.ok) return;

            const data = await response.json();

            // Update status bar
            this.updateStatusField('wifi-status', data.wifi_status, 'WiFi');
            this.updateStatusField('mqtt-status', data.mqtt_status, 'MQTT');
            this.updateUptimeField('uptime-status', data.uptime_ms);

            // Update system cards
            const heapFree = (data.heap_free / 1024).toFixed(1);
            const heapTotal = (data.heap_total / 1024).toFixed(1);
            this.setElementText('heap-free', heapFree + ' KB');
            this.setElementText('heap-total', heapTotal + ' KB');
            this.setElementText('signals-count', `${data.signals_loaded}/${data.signals_max}`);
            this.setElementText('firmware-version', data.firmware);

            // Power monitor card
            if (data.power && data.power.available) {
                this.setElementText(
                    'power-info',
                    `${data.power.bus_voltage_v.toFixed(2)} V · ${data.power.current_ma.toFixed(0)} mA`
                );
            } else {
                this.setElementText('power-info', 'N/A');
            }
        } catch (error) {
            console.error('[App] System info update failed:', error);
        }
    }

    /**
     * Update status field
     */
    updateStatusField(elementId, status, label) {
        const element = document.getElementById(elementId);
        if (!element) return;

        const statusText = document.querySelector(`#${elementId} .status-text`);
        if (statusText) {
            statusText.textContent = `${label}: ${status}`;

            // Update colors based on status
            element.style.color =
                status === 'connected'
                    ? '#4CAF50'
                    : status === 'connecting'
                      ? '#ff9800'
                      : '#f44336';
        }
    }

    /**
     * Update uptime field
     */
    updateUptimeField(elementId, uptimeMs) {
        const element = document.getElementById(elementId);
        if (!element) return;

        const seconds = Math.floor(uptimeMs / 1000);
        const minutes = Math.floor(seconds / 60);
        const hours = Math.floor(minutes / 60);
        const days = Math.floor(hours / 24);

        let uptime;
        if (days > 0) {
            uptime = `${days}d ${hours % 24}h`;
        } else if (hours > 0) {
            uptime = `${hours}h ${minutes % 60}m`;
        } else if (minutes > 0) {
            uptime = `${minutes}m ${seconds % 60}s`;
        } else {
            uptime = `${seconds}s`;
        }

        const statusText = element.querySelector('.status-text');
        if (statusText) {
            statusText.textContent = `Uptime: ${uptime}`;
        }
    }

    /**
     * Update status bar
     */
    updateStatusBar() {
        const grid = document.getElementById('signals-grid');
        if (grid && this.signals.length > 0) {
            grid.style.display = 'grid';
        }
    }

    /**
     * Attach event listeners
     */
    attachEventListeners() {
        // Refresh button
        document.getElementById('refresh-btn')?.addEventListener('click', () => {
            this.loadSignals();
            this.loadConfig();
        });

        // Restart button
        document.getElementById('restart-btn')?.addEventListener('click', () => {
            if (
                confirm(
                    'Are you sure you want to restart the device? You will lose connection briefly.'
                )
            ) {
                this.restartDevice();
            }
        });

        // OTA upload buttons
        document.getElementById('ota-firmware-btn')?.addEventListener('click', () => {
            this.uploadOta('/api/ota/firmware', 'ota-firmware-file', 'firmware');
        });
        document.getElementById('ota-fs-btn')?.addEventListener('click', () => {
            this.uploadOta('/api/ota/filesystem', 'ota-fs-file', 'filesystem');
        });

        // Toast close buttons
        document.querySelectorAll('.toast-close').forEach((btn) => {
            btn.addEventListener('click', (e) => {
                e.target.closest('.toast').classList.remove('show');
            });
        });
    }

    /**
     * Restart device
     */
    async restartDevice() {
        try {
            const response = await fetch(
                `${this.apiBase}/api/system/restart`,
                { method: 'POST' }
            );

            if (response.ok) {
                this.showSuccess('Device restarting...');
                setTimeout(() => {
                    window.location.reload();
                }, 2000);
            }
        } catch (error) {
            console.error('[App] Restart failed:', error);
            this.showError('Failed to restart device');
        }
    }

    /**
     * Show error notification
     */
    showError(message) {
        const toast = document.getElementById('error-toast');
        if (toast) {
            document.getElementById('error-message').textContent = message;
            toast.classList.add('show');
            setTimeout(() => {
                toast.classList.remove('show');
            }, 4000);
        }
    }

    /**
     * Show success notification
     */
    showSuccess(message) {
        const toast = document.getElementById('success-toast');
        if (toast) {
            document.getElementById('success-message').textContent = message;
            toast.classList.add('show');
            setTimeout(() => {
                toast.classList.remove('show');
            }, 3000);
        }
    }

    /**
     * Helper: Set element text
     */
    setElementText(elementId, text) {
        const element = document.getElementById(elementId);
        if (element) {
            element.textContent = text;
        }
    }
}

// Initialize app when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.app = new SignalBoardApp();
});

// Cleanup on page unload
window.addEventListener('unload', () => {
    if (window.app && window.app.updateInterval) {
        clearInterval(window.app.updateInterval);
    }
});
