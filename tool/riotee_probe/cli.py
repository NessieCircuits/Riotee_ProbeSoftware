import sys
from contextlib import contextmanager
from pathlib import Path
from typing import Generator

import click
from progress.bar import Bar

from .probe import GpioDir
from .session import get_connected_probe
from .target import Target
from .session import get_all_probe_sessions

device_option = click.option("-d", "--device", type=click.Choice(["msp430", "nrf52"]), default="nrf52")


@contextmanager
def get_target(device: str) -> Generator[Target, None, None]:
    with get_connected_probe() as probe:
        if device == "nrf52":
            target_gen = probe.nrf52
        else:
            target_gen = probe.msp430

        with target_gen() as target:
            yield target


@click.group
def cli() -> None:
    pass


@cli.command(short_help="Control power supply bypass (Board only)")
@click.option("--on/--off", required=True)
@click.pass_context
def bypass(ctx: click.Context, on: bool) -> None:
    with get_connected_probe() as probe:
        probe.bypass(on)


@cli.command(short_help="Control target power supply")
@click.option("--on/--off", required=True)
@click.pass_context
def target_power(ctx: click.Context, on: bool) -> None:
    with get_connected_probe() as probe:
        probe.target_power(on)


@cli.command
@device_option
@click.option("-f", "--firmware", type=click.Path(exists=True), required=True)
def program(device: str, firmware: Path) -> None:
    with get_target(device) as target:
        bar = Bar("Uploading..", max=100)

        def update_bar(fraction: float) -> None:
            bar.goto(100 * fraction)

        target.program(firmware, progress=update_bar)
        bar.finish()


@cli.command
@device_option
def reset(device: str) -> None:
    with get_target(device) as target:
        target.reset()


@cli.command
@device_option
def halt(device: str) -> None:
    with get_target(device) as target:
        target.halt()


@cli.command
@device_option
def resume(device: str) -> None:
    with get_target(device) as target:
        target.resume()


@cli.group(short_help="Control GPIOs (Probe only)")
def gpio() -> None:
    pass


@gpio.command(short_help="Configure GPIO pin direction")
@click.option("--pin-no", "-p", type=int, required=True, help="Pin number")
@click.option(
    "--direction",
    "-d",
    type=click.Choice(["in", "out"], case_sensitive=False),
    required=True,
    help="Pin direction",
)
def dir(pin_no: int, direction: str) -> None:
    with get_connected_probe() as probe:
        if direction == "in":
            probe.gpio_dir(pin_no, GpioDir.GPIO_DIR_IN)
        else:
            probe.gpio_dir(pin_no, GpioDir.GPIO_DIR_OUT)


@gpio.command(short_help="Set GPIO pin")
@click.option("--pin-no", "-p", type=int, required=True, help="Pin number")
@click.option(
    "--state",
    "-s",
    type=click.Choice(["high", "1", "low", "0"], case_sensitive=False),
    required=True,
    help="Pin state",
)
def set(pin_no: int, state: str) -> None:
    with get_connected_probe() as probe:
        if state in ["high", "1"]:
            probe.gpio_set(pin_no, True)
        else:
            probe.gpio_set(pin_no, False)


@gpio.command(short_help="Read GPIO pin")
@click.option("--pin-no", "-p", type=int, required=True, help="Pin number")
def get(pin_no: int) -> None:
    with get_connected_probe() as probe:
        state = probe.gpio_get(pin_no)
        click.echo(state)


@cli.command
def list() -> None:
    """Show any connected device and its firmware version"""

    printed = False
    for details in get_all_probe_sessions():
        if not printed:
            click.echo(" ".join("%-20s" % key for key in details))
            printed = True
        click.echo(" ".join("%-20s" % v for v in details.values()))
    if not printed:
        click.echo("Currently no probes connected", err=True)


if __name__ == "__main__":
    cli()
