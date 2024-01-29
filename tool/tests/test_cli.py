from click.testing import CliRunner
from riotee_probe.cli import cli


def test_cli_list_probes(
    cli_runner: CliRunner,
) -> None:
    res = cli_runner.invoke(cli, ["list"])
    assert res.exit_code == 0


def test_cli_print_version(
    cli_runner: CliRunner,
) -> None:
    res = cli_runner.invoke(cli, ["--version", "list"])
    assert res.exit_code == 0


# TODO: add more tests - but these will need actual hardware
