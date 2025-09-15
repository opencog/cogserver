from pkgutil import extend_path
__path__ = extend_path(__path__, __name__)

# Import and expose cogserver functions
try:
    from .cogserver import (
        start_cogserver,
        stop_cogserver,
        is_cogserver_running,
        get_server_atomspace
    )
    __all__ = [
        'start_cogserver',
        'stop_cogserver',
        'is_cogserver_running',
        'get_server_atomspace'
    ]
except ImportError:
    # Module not built yet
    pass
