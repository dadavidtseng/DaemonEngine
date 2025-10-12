//----------------------------------------------------------------------------------------------------
// Event.js - Base Event Class for Type-Safe Event System
//----------------------------------------------------------------------------------------------------

/**
 * Event - Abstract base class for all event types
 *
 * Features:
 * - Standard event structure (type, timestamp, data)
 * - Type safety through inheritance
 * - Debugging support (toString, toJSON)
 * - Immutable event type
 * - Cancellation support (optional, for future use)
 *
 * Usage:
 * ```javascript
 * // Create specific event types by extending this class
 * class GameStateChangedEvent extends Event {
 *     constructor(oldState, newState) {
 *         super('GameStateChanged');
 *         this.oldState = oldState;
 *         this.newState = newState;
 *     }
 * }
 * ```
 */
export class Event
{
    /**
     * Create a new event
     * @param {string} type - Event type identifier (must match EventTypes constant)
     * @param {object} data - Additional event data (optional)
     */
    constructor(type, data = {})
    {
        // Prevent direct instantiation of Event class (abstract class pattern)
        if (new.target === Event)
        {
            throw new Error('Event is an abstract class and cannot be instantiated directly. Extend it instead.');
        }

        // Validate event type
        if (typeof type !== 'string' || type.length === 0)
        {
            throw new Error('Event type must be a non-empty string');
        }

        // Core event properties (immutable)
        Object.defineProperty(this, 'type', {
            value: type,
            writable: false,
            enumerable: true,
            configurable: false
        });

        Object.defineProperty(this, 'timestamp', {
            value: Date.now(),
            writable: false,
            enumerable: true,
            configurable: false
        });

        // Store additional data
        this.data = data;

        // Event cancellation support (optional for future use)
        this._cancelled = false;
    }

    /**
     * Check if event is cancelled
     * @returns {boolean} True if event has been cancelled
     */
    get isCancelled()
    {
        return this._cancelled;
    }

    /**
     * Cancel this event (prevents further processing)
     * Note: Cancellation support is optional and must be implemented by event handlers
     */
    cancel()
    {
        this._cancelled = true;
    }

    /**
     * Get human-readable event description
     * @returns {string} Event description
     */
    toString()
    {
        return `Event[${this.type}] at ${new Date(this.timestamp).toISOString()}`;
    }

    /**
     * Convert event to JSON (for logging/debugging)
     * @returns {object} JSON representation
     */
    toJSON()
    {
        return {
            type: this.type,
            timestamp: this.timestamp,
            data: this.data,
            cancelled: this._cancelled
        };
    }

    /**
     * Get elapsed time since event creation
     * @returns {number} Milliseconds elapsed
     */
    getAge()
    {
        return Date.now() - this.timestamp;
    }
}

console.log('Event: Base class loaded (Abstract Event pattern)');
