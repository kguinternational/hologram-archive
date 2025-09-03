//! Error handling for Atlas Manifold operations

/// Atlas error types for manifold operations
#[derive(Debug, Clone, PartialEq)]
pub enum AtlasError {
    /// Invalid dimension specification
    InvalidDimension(u32),
    /// Matrix operation error
    MatrixError(&'static str),
    /// Manifold topology error
    TopologyError(&'static str),
    /// Coordinate transformation error
    CoordinateError(&'static str),
    /// Serialization/deserialization error
    SerializationError(&'static str),
    /// Memory allocation error
    AllocationError,
    /// Invalid input parameters
    InvalidInput(&'static str),
    /// Numerical computation error
    NumericalError(&'static str),
    /// Integration with lower layers failed
    LayerIntegrationError(&'static str),
}

impl core::fmt::Display for AtlasError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            AtlasError::InvalidDimension(dim) => 
                write!(f, "Invalid dimension: {}", dim),
            AtlasError::MatrixError(msg) => 
                write!(f, "Matrix operation error: {}", msg),
            AtlasError::TopologyError(msg) => 
                write!(f, "Manifold topology error: {}", msg),
            AtlasError::CoordinateError(msg) => 
                write!(f, "Coordinate transformation error: {}", msg),
            AtlasError::SerializationError(msg) => 
                write!(f, "Serialization error: {}", msg),
            AtlasError::AllocationError => 
                write!(f, "Memory allocation failed"),
            AtlasError::InvalidInput(msg) => 
                write!(f, "Invalid input: {}", msg),
            AtlasError::NumericalError(msg) => 
                write!(f, "Numerical computation error: {}", msg),
            AtlasError::LayerIntegrationError(msg) => 
                write!(f, "Layer integration error: {}", msg),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for AtlasError {}

/// Result type for Atlas manifold operations
pub type AtlasResult<T> = Result<T, AtlasError>;

/// Convert error to C-compatible error code
pub fn error_to_code(error: &AtlasError) -> i32 {
    match error {
        AtlasError::InvalidDimension(_) => -1,
        AtlasError::MatrixError(_) => -2,
        AtlasError::TopologyError(_) => -3,
        AtlasError::CoordinateError(_) => -4,
        AtlasError::SerializationError(_) => -5,
        AtlasError::AllocationError => -6,
        AtlasError::InvalidInput(_) => -7,
        AtlasError::NumericalError(_) => -8,
        AtlasError::LayerIntegrationError(_) => -9,
    }
}