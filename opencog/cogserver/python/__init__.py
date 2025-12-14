from pkgutil import extend_path
__path__ = extend_path(__path__, __name__)

# Import and expose cogserver functions
try:
    from .cogserver import (
        start_cogserver,
        stop_cogserver,
    )
    __all__ = [
        'start_cogserver',
        'stop_cogserver',
    ]
except ImportError:
    # Module not fully installed yet
    pass
