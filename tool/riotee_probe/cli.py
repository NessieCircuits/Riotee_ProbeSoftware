from contextlib import contextmanager
import click
from progress.bar import Bar
from riotee_probe import RioteeProbe
from riotee_probe import RioteeProbeProbe

device_option = click.option("-d", "--device", type=click.Choice(["msp430", "nrf52"]), default="nrf52")


@contextmanager
def get_target(device):
    with RioteeProbe() as probe:
        if device == "nrf52":
            target_gen = probe.nrf52
        else:
            target_gen = probe.msp430

        with target_gen() as target:
            yield target


@click.group
def cli():
    pass


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


if __name__ == "__main__":
    cli()
