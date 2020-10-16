"""ONVIF event abstraction."""
import asyncio
import datetime as dt
from typing import Callable, Dict, List, Optional, Set

from aiohttp.client_exceptions import (
    ClientConnectorError,
    ClientOSError,
    ServerDisconnectedError,
)
from onvif import ONVIFCamera, ONVIFService
from zeep.exceptions import Fault

from homeassistant.core import CALLBACK_TYPE, CoreState, HomeAssistant, callback
from homeassistant.helpers.event import async_call_later
from homeassistant.util import dt as dt_util

from .const import LOGGER
from .models import Event
from .parsers import PARSERS

UNHANDLED_TOPICS = set()
SUBSCRIPTION_ERRORS = (
    ClientConnectorError,
    ClientOSError,
    Fault,
    ServerDisconnectedError,
    asyncio.TimeoutError,
)


class EventManager:
    """ONVIF Event Manager."""

    def __init__(self, hass: HomeAssistant, device: ONVIFCamera, unique_id: str):
        """Initialize event manager."""
        self.hass: HomeAssistant = hass
        self.device: ONVIFCamera = device
        self.unique_id: str = unique_id
        self.started: bool = False

        self._subscription: ONVIFService = None
        self._events: Dict[str, Event] = {}
        self._listeners: List[CALLBACK_TYPE] = []
        self._unsub_refresh: Optional[CALLBACK_TYPE] = None

        super().__init__()

    @property
    def platforms(self) -> Set[str]:
        """Return platforms to setup."""
        return {event.platform for event in self._events.values()}

    @callback
    def async_add_listener(self, update_callback: CALLBACK_TYPE) -> Callable[[], None]:
        """Listen for data updates."""
        # This is the first listener, set up polling.
        if not self._listeners:
            self.async_schedule_pull()

        self._listeners.append(update_callback)

        @callback
        def remove_listener() -> None:
            """Remove update listener."""
            self.async_remove_listener(update_callback)

        return remove_listener

    @callback
    def async_remove_listener(self, update_callback: CALLBACK_TYPE) -> None:
        """Remove data update."""
        if update_callback in self._listeners:
            self._listeners.remove(update_callback)

        if not self._listeners and self._unsub_refresh:
            self._unsub_refresh()
            self._unsub_refresh = None

    async def async_start(self) -> bool:
        """Start polling events."""
        if await self.device.create_pullpoint_subscription():
            # Initialize events
            pullpoint = self.device.create_pullpoint_service()
            await pullpoint.SetSynchronizationPoint()
            req = pullpoint.create_type("PullMessages")
            req.MessageLimit = 100
            req.Timeout = dt.timedelta(seconds=5)
            response = await pullpoint.PullMessages(req)

            # Parse event initialization
            await self.async_parse_messages(response.NotificationMessage)

            # Create subscription manager
            self._subscription = self.device.create_subscription_service(
                "PullPointSubscription"
            )

            self.started = True
            return True

        return False

    async def async_stop(self) -> None:
        """Unsubscribe from events."""
        self._listeners = []
        self.started = False

        if not self._subscription:
            return

        await self._subscription.Unsubscribe()
        self._subscription = None

    async def async_restart(self, _now: dt = None) -> None:
        """Restart the subscription assuming the camera rebooted."""
        if not self.started:
            return

        if self._subscription:
            try:
                await self._subscription.Unsubscribe()
            except SUBSCRIPTION_ERRORS:
                pass  # Ignored. The subscription may no longer exist.
            self._subscription = None

        try:
            restarted = await self.async_start()
        except SUBSCRIPTION_ERRORS:
            restarted = False

        if not restarted:
            LOGGER.warning(
                "Failed to restart ONVIF PullPoint subscription for '%s'. Retrying...",
                self.unique_id,
            )
            # Try again in a minute
            self._unsub_refresh = async_call_later(self.hass, 60, self.async_restart)
        elif self._listeners:
            LOGGER.debug(
                "Restarted ONVIF PullPoint subscription for '%s'", self.unique_id
            )
            self.async_schedule_pull()

    async def async_renew(self) -> None:
        """Renew subscription."""
        if not self._subscription:
            return

        termination_time = (
            (dt_util.utcnow() + dt.timedelta(days=1))
            .isoformat(timespec="seconds")
            .replace("+00:00", "Z")
        )
        await self._subscription.Renew(termination_time)

    def async_schedule_pull(self) -> None:
        """Schedule async_pull_messages to run."""
        self._unsub_refresh = async_call_later(self.hass, 1, self.async_pull_messages)

    async def async_pull_messages(self, _now: dt = None) -> None:
        """Pull messages from device."""
        if self.hass.state == CoreState.running:
            try:
                pullpoint = self.device.create_pullpoint_service()
                req = pullpoint.create_type("PullMessages")
                req.MessageLimit = 100
                req.Timeout = dt.timedelta(seconds=60)
                response = await pullpoint.PullMessages(req)

                # Renew subscription if less than two hours is left
                if (
                    dt_util.as_utc(response.TerminationTime) - dt_util.utcnow()
                ).total_seconds() < 7200:
                    await self.async_renew()
            except SUBSCRIPTION_ERRORS:
                LOGGER.warning(
                    "Failed to fetch ONVIF PullPoint subscription messages for '%s'",
                    self.unique_id,
                )
                # Treat errors as if the camera restarted. Assume that the pullpoint
                # subscription is no longer valid.
                self._unsub_refresh = None
                await self.async_restart()
                return

            # Parse response
            await self.async_parse_messages(response.NotificationMessage)

            # Update entities
            for update_callback in self._listeners:
                update_callback()

        # Reschedule another pull
        if self._listeners:
            self.async_schedule_pull()

    # pylint: disable=protected-access
    async def async_parse_messages(self, messages) -> None:
        """Parse notification message."""
        for msg in messages:
            # Guard against empty message
            if not msg.Topic:
                continue

            topic = msg.Topic._value_1
            parser = PARSERS.get(topic)
            if not parser:
                if topic not in UNHANDLED_TOPICS:
                    LOGGER.info(
                        "No registered handler for event from %s: %s",
                        self.unique_id,
                        msg,
                    )
                    UNHANDLED_TOPICS.add(topic)
                continue

            event = await parser(self.unique_id, msg)

            if not event:
                LOGGER.warning("Unable to parse event from %s: %s", self.unique_id, msg)
                return

            self._events[event.uid] = event

    def get_uid(self, uid) -> Event:
        """Retrieve event for given id."""
        return self._events[uid]

    def get_platform(self, platform) -> List[Event]:
        """Retrieve events for given platform."""
        return [event for event in self._events.values() if event.platform == platform]
