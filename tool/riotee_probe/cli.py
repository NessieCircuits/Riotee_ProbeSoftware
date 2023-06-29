from contextlib import contextmanager
import click
from progress.bar import Bar
from riotee_probe.probe import get_connected_probe
from riotee_probe.probe import GpioDir

device_option = click.option("-d", "--device", type=click.Choice(["msp430", "nrf52"]), default="nrf52")


@contextmanager
def get_target(device):
    with get_connected_probe() as probe:
        if device == "nrf52":
            target_gen = probe.nrf52
        else:
            target_gen = probe.msp430

        with target_gen() as target:
            yield target


@click.group
def cli():
    pass


@cli.command(short_help="Control power supply bypass")
@click.option("--on/--off", required=True)
@click.pass_context
def bypass(ctx, on):
    with get_connected_probe() as probe:
        probe.bypass(on)


@cli.command(short_help="Control target power supply")
@click.option("--on/--off", required=True)
@click.pass_context
def target_power(ctx, on):
    with get_connected_probe() as probe:
        probe.target_power(on)


@cli.command
@device_option
@click.option("-f", "--firmware", type=click.Path(exists=True), required=True)
def program(device, firmware):
    with get_target(device) as target:
        bar = Bar("Uploading..", max=100)

        def update_bar(fraction):
            bar.goto(100 * fraction)

        target.program(firmware, progress=update_bar)
        bar.finish()


@cli.command
@device_option
def reset(device):
    with get_target(device) as target:
        target.reset()


@cli.command
@device_option
def halt(device):
    with get_target(device) as target:
        target.halt()


@cli.command
@device_option
def resume(device):
    with get_target(device) as target:
        target.resume()


@cli.command(short_help="Configure GPIO pin direction")
@click.option("--pin-no", "-p", type=int, required=True, help="Pin number")
@click.option(
    "--direction",
    "-d",
    type=click.Choice(["in", "out"], case_sensitive=False),
    required=True,
    help="Pin direction",
)
def gpio_dir(pin_no, direction):
    with get_connected_probe() as probe:
        if direction == "in":
            probe.gpio_dir(pin_no, GpioDir.GPIO_DIR_IN)
        else:
            probe.gpio_dir(pin_no, GpioDir.GPIO_DIR_OUT)


@cli.command(short_help="Set GPIO pin")
@click.option("--pin-no", "-p", type=int, required=True, help="Pin number")
@click.option(
    "--state",
    "-s",
    type=click.Choice(["high", "1", "low", "0"], case_sensitive=False),
    required=True,
    help="Pin state",
)
def gpio_set(pin_no, state):
    with get_connected_probe() as probe:
        if state in ["high", "1"]:
            probe.gpio_set(pin_no, True)
        else:
            probe.gpio_set(pin_no, False)


@cli.command(short_help="Read GPIO pin")
@click.option("--pin-no", "-p", type=int, required=True, help="Pin number")
def gpio_get(pin_no):
    with get_connected_probe() as probe:
        state = probe.gpio_get(pin_no)
        click.echo(state)


if __name__ == "__main__":
    cli()
